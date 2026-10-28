#!/usr/bin/env python3
"""Capability-checked USB management for the SynthDef firmware only."""
import argparse,json,secrets,struct,time,zlib
from pathlib import Path
import mido
PREFIX=bytes([0x7d,0x46,0x4d,0x53,1])
def pack7(data):
 out=bytearray()
 for start in range(0,len(data),7):
  group=data[start:start+7];out.append(sum((b>>7)<<i for i,b in enumerate(group)));out.extend(b&127 for b in group)
 return out
def unpack7(data):
 out=bytearray()
 for start in range(0,len(data),8):
  h=data[start];g=data[start+1:start+8]
  if not g or h>>len(g):raise ValueError('Malformed MIDI packing')
  out.extend(b|((h>>i&1)<<7) for i,b in enumerate(g))
 return bytes(out)
class Device:
 def __init__(self,port=None):
  names=mido.get_output_names();self.port=port or next((n for n in names if 'FM-1' in n),None)
  if not self.port:raise ValueError('No FM-1 USB MIDI port')
  self.inp=mido.open_input(self.port);self.out=mido.open_output(self.port);self.serial=secrets.randbelow(65536)
  try:
   self.info=self.request(0)
   if len(self.info)!=19 or self.info[:2]!=bytes([2,9]):raise ValueError('Device is not compatible SynthDef firmware')
  except Exception:
   self.close();raise
 def close(self):self.inp.close();self.out.close()
 def request(self,command,payload=b''):
  self.serial=(self.serial+1)&65535
  raw=struct.pack('<BBHH',1,command,self.serial,len(payload))+payload;raw+=struct.pack('<I',zlib.crc32(raw))
  msg=mido.Message('sysex',data=PREFIX+pack7(raw))
  for attempt in range(3):
   self.out.send(msg);end=time.monotonic()+5
   while time.monotonic()<end:
    for m in self.inp.iter_pending():
     if m.type!='sysex' or bytes(m.data[:5])!=PREFIX:continue
     reply=unpack7(bytes(m.data[5:]))
     if len(reply)<11:continue
     v,cmd,ident,size=struct.unpack_from('<BBHH',reply)
     if v!=1 or cmd!=command|128 or ident!=self.serial:continue
     if len(reply)!=size+10 or struct.unpack_from('<I',reply,len(reply)-4)[0]!=zlib.crc32(reply[:-4]):raise ValueError('Reply checksum mismatch')
     if reply[6]:raise ValueError('Device rejected request: status '+str(reply[6]))
     return reply[7:-4]
    time.sleep(0.002)
  raise TimeoutError('FM-1 did not acknowledge request')
 def read(self,slot):
  data=bytearray();generation=0xffffffff
  while True:
   r=self.request(1,struct.pack('<BIHI',slot,len(data),192,generation));g,n,offset=struct.unpack_from('<III',r)
   if offset!=len(data) or n>16128:raise ValueError('Invalid read bounds')
   generation=g;data.extend(r[12:])
   if len(data)==n:return bytes(data),g
   if len(r)==12:raise ValueError('Truncated slot')
 def write(self,slot,data,expected=None):
  if not 0<=slot<9 or len(data)>(7936 if slot==8 else 16128):raise ValueError('Invalid slot/package size')
  old,generation=self.read(slot)
  if expected is not None and expected!=generation:raise ValueError("FM-1 slot changed. Check the device again before sending.")
  tx=secrets.randbits(32)
  self.request(2,struct.pack('<BIIII',slot,generation,len(data),zlib.crc32(data),tx))
  for offset in range(0,len(data),192):self.request(3,struct.pack('<II',tx,offset)+data[offset:offset+192])
  self.request(4,struct.pack('<I',tx))
  actual,revision=self.read(slot)
  if actual!=data or revision!=generation+1:raise ValueError('Slot readback did not match')
  return revision
 def stats(self,benchmark=False):
  code=0x7e if benchmark else 0x7f
  # Telemetry is read-only and may retry a dropped USB reply. Benchmarking
  # stops/reloads the patch, so it must never be repeated implicitly.
  for attempt in range(1 if benchmark else 3):
   self.out.send(mido.Message('sysex',data=PREFIX+bytes([code])));end=time.monotonic()+(3 if benchmark else 0.15)
   while time.monotonic()<end:
    for m in self.inp.iter_pending():
     p=bytes(m.data) if m.type=='sysex' else b''
     if len(p) in (46,66,76) and p[:6]==PREFIX+bytes([code]):
      values=[sum(p[6+i*5+j]<<(7*j) for j in range(5)) for i in range((len(p)-6)//5)]
      if benchmark:return dict(zip(['init_one_us','first_one_us','steady_one_us','init_two_us','first_two_us','steady_two_us','faults','loaded'],values))
      return dict(zip(['slot_index','loaded','cpu_percent','xruns','faults','overloads','active_voices','notes_started','render_peak_us','render_last_us','start_peak_us','steady_peak_us','view_slot_index','effect_active'],values))
    time.sleep(0.002)
  raise TimeoutError('No device telemetry')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('action',choices=['info','read','write','clear','stats','benchmark']);p.add_argument('--slot',type=int,default=1);p.add_argument('--file',type=Path);p.add_argument('--port');a=p.parse_args()
 if not 0<=a.slot<=8:p.error('Slot is 0–8 (0 is the permanent effect)')
 if a.action in ('read','write') and not a.file:p.error('--file is required')
 dev=Device(a.port)
 try:
  if a.action=='info':print(json.dumps(dict(port=dev.port,slots=dev.info[1],effect_capacity=struct.unpack_from('<I',dev.info,15)[0],voices=dev.info[2],capacity=struct.unpack_from('<I',dev.info,3)[0])))
  elif a.action in ('stats','benchmark'):print(json.dumps(dev.stats(a.action=='benchmark')))
  elif a.action=='read':data,rev=dev.read((a.slot-1)%9);a.file.write_bytes(data);print('Read slot',a.slot,'revision',rev,'bytes',len(data))
  else:print('Verified slot',a.slot,'revision',dev.write((a.slot-1)%9,b'' if a.action=='clear' else a.file.read_bytes()))
 finally:dev.close()
if __name__=='__main__':main()
