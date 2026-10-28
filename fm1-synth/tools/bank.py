"""Local flash-image adapter and bounded slot packages. Uses the device C journal."""
import array
import base64
import ctypes as C
import hashlib
import io
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import wave
import zlib

ROOT=Path(__file__).resolve().parents[1]
CAPACITY=16128
FX_SLOT=8
FX_CAPACITY=7936
SLOT_ORDER=[8,*range(8)]
def capacity(slot):return FX_CAPACITY if slot==FX_SLOT else CAPACITY
SCLANG=Path('/Applications/SuperCollider.app/Contents/MacOS/sclang')
import sys
sys.path.insert(0,str(ROOT.parent/'synthdef-engine/tools'))
from sc_language import language_command
LIB=C.CDLL(str(ROOT/'build/libinstrument.dylib'))
for name in ('fms_graph_size','fms_performance_size','fms_voice_size'):
    getattr(LIB,name).restype=C.c_size_t
LIB.fm_load.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t,C.c_char_p,C.c_size_t]
LIB.fm_delay_required.argtypes=[C.c_void_p,C.c_float];LIB.fm_delay_required.restype=C.c_uint32
LIB.fms_init.argtypes=[C.c_void_p,C.c_void_p]
LIB.fms_effect_valid.argtypes=[C.c_void_p]
LIB.fms_attach_effect.argtypes=[C.c_void_p,C.c_void_p,C.c_void_p]
LIB.fms_reset_effect.argtypes=[C.c_void_p]
LIB.fms_note_on.argtypes=[C.c_void_p,C.c_uint32,C.c_uint,C.c_uint]
LIB.fms_note_off.argtypes=[C.c_void_p,C.c_uint32]
LIB.fms_knob.argtypes=[C.c_void_p,C.c_uint,C.c_float]
LIB.fms_render.argtypes=[C.c_void_p,C.c_void_p]
LIB.fms_faults.argtypes=[C.c_void_p]
for name in ('fms_name','fms_control_name'):
    getattr(LIB,name).restype=C.c_char_p
LIB.fms_name.argtypes=[C.c_void_p]
LIB.fms_controls.argtypes=LIB.fms_nodes.argtypes=[C.c_void_p]
LIB.fms_control_name.argtypes=LIB.fms_control_default.argtypes=[C.c_void_p,C.c_uint]
LIB.fms_control_default.restype=C.c_float
READ=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32,C.c_void_p,C.c_uint32)
ERASE=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32)
PROGRAM=C.CFUNCTYPE(C.c_int,C.c_void_p,C.c_uint32,C.c_void_p,C.c_uint32)
class Flash(C.Structure):
    _fields_=[('context',C.c_void_p),('read',READ),('erase',ERASE),('program',PROGRAM)]
LIB.fms_slot_read.argtypes=[C.POINTER(Flash),C.c_uint,C.c_void_p,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32)]
LIB.fms_slot_write.argtypes=[C.POINTER(Flash),C.c_uint,C.c_void_p,C.c_uint32,C.c_uint32,C.c_void_p]

def validate(binary,effect=False):
    if not isinstance(binary,bytes) or not 0<len(binary)<=65536:raise ValueError('Choose one compiled SynthDef, at most 64 KiB.')
    graph=C.create_string_buffer(LIB.fms_graph_size());error=C.create_string_buffer(256)
    if not LIB.fm_load(graph,binary,len(binary),error,len(error)):raise ValueError(error.value.decode())
    samples=LIB.fm_delay_required(graph,44100)
    if samples>(32768 if effect else 16384):raise ValueError('Delay/reverb memory budget exceeded. Reduce delay lengths or effects; prefer slot 0 for a shared reverb.')
    controls={LIB.fms_control_name(graph,i).decode():LIB.fms_control_default(graph,i) for i in range(LIB.fms_controls(graph))}
    check=C.create_string_buffer(LIB.fms_performance_size())
    if effect:
        if not LIB.fms_effect_valid(graph):raise ValueError('Slot 0 requires a continuous effect (doneAction 0) and control-rate knob arguments.')
    elif not LIB.fms_init(check,graph):raise ValueError('Playable synths cannot use In; knob1–knob4 must be control-rate or lagged controls.')
    return graph,dict(name=LIB.fms_name(graph).decode(),nodes=LIB.fms_nodes(graph),controls=controls,delay_bytes=samples*4,role='effect' if effect else 'synth')

def digest(data):return hashlib.sha256(data).hexdigest()
def pack(name,source,binary,effect=False):
    if not isinstance(name,str) or not 1<=len(name.encode())<=48 or any(ord(c)<32 for c in name):raise ValueError('Name must be 1–48 UTF-8 bytes, without control characters.')
    if not isinstance(source,str) or '\0' in source:raise ValueError('Source must be text without NUL characters.')
    validate(binary,effect)
    text=source.encode('utf-8')
    meta=json.dumps(dict(version=1,role='effect' if effect else 'synth',name=name,source_sha256=digest(text),binary_sha256=digest(binary)),separators=(',',':'),ensure_ascii=False).encode()
    body=meta+text+binary
    payload=struct.pack('<4sIIII',b'FMB1',len(meta),len(text),len(binary),zlib.crc32(body))+body
    limit=FX_CAPACITY if effect else CAPACITY
    if len(payload)>limit:raise ValueError(f'Slot needs {len(payload):,} bytes; capacity is {limit:,}. Shorten source or simplify the graph.')
    return payload
def unpack(payload):
    if len(payload)<20 or len(payload)>CAPACITY:raise ValueError('Invalid slot package size.')
    magic,m,s,b,crc=struct.unpack_from('<4sIIII',payload)
    if magic!=b'FMB1' or 20+m+s+b!=len(payload):raise ValueError('Invalid slot package lengths.')
    if crc!=zlib.crc32(payload[20:]):raise ValueError('Slot package CRC mismatch.')
    meta=json.loads(payload[20:20+m]);source=payload[20+m:20+m+s].decode('utf-8');binary=payload[20+m+s:]
    if meta.get('version')!=1 or meta.get('source_sha256')!=digest(source.encode()) or meta.get('binary_sha256')!=digest(binary):raise ValueError('Slot package checksum/version mismatch.')
    # Canonical repack also checks metadata and loader limits.
    pack(meta['name'],source,binary,meta.get('role')=='effect')
    return meta['name'],source,binary

class Bank:
    def __init__(self,path):
        self.path=Path(path);self.path.parent.mkdir(parents=True,exist_ok=True)
        if not self.path.exists():
            with self.path.open('xb') as f:f.write(b'\xff'*0x100000);f.flush();os.fsync(f.fileno())
        self.data=bytearray(self.path.read_bytes())
        if len(self.data)!=0x100000:raise ValueError('Local flash image has the wrong size; preserved without modification.')
        self.callbacks=(READ(self.read),ERASE(self.erase),PROGRAM(self.program))
        self.flash=Flash(None,*self.callbacks)
    def _range(self,off,n):return 0x97000<=off<=off+n<=0xDB000
    def read(self,ctx,off,dst,n):
        if not self._range(off,n):return -1
        C.memmove(dst,bytes(self.data[off:off+n]),n);return 0
    def _write(self,off,data):
        try:
            with self.path.open('r+b') as f:f.seek(off);f.write(data);f.flush();os.fsync(f.fileno())
            self.data[off:off+len(data)]=data;return 0
        except OSError:return -1
    def erase(self,ctx,off):
        if off%4096 or not self._range(off,4096):return -1
        return self._write(off,b'\xff'*4096)
    def program(self,ctx,off,src,n):
        if not n or n>256 or off//256!=(off+n-1)//256 or not self._range(off,n):return -1
        data=C.string_at(src,n)
        if any((a&b)!=b for a,b in zip(self.data[off:off+n],data)):return -1
        return self._write(off,data)
    def get(self,slot):
        if type(slot)!=int or not 0<=slot<9:raise ValueError('Slot must be 0–8 (0 is the permanent effect).' )
        buf=C.create_string_buffer(CAPACITY);length=C.c_uint32();generation=C.c_uint32()
        if LIB.fms_slot_read(C.byref(self.flash),slot,buf,C.byref(length),C.byref(generation)):raise ValueError('Could not read slot.')
        return buf.raw[:length.value],generation.value
    def put(self,slot,payload,expected):
        if type(expected)!=int or not 0<=expected<2**32-1:raise ValueError('Invalid slot revision.')
        self.get(slot)
        if payload:
            if len(payload)>capacity(slot):raise ValueError('Slot capacity exceeded.')
            _,_,binary=unpack(payload);_,info=validate(binary,slot==FX_SLOT)
            if slot==FX_SLOT:
                fx_bytes=info['delay_bytes'];voice_slots=range(8)
            else:
                fx,_=self.get(FX_SLOT);fx_bytes=validate(unpack(fx)[2],True)[1]['delay_bytes'] if fx else 0;voice_slots=[slot]
            for index in voice_slots:
                body,_=self.get(index)
                voice_bytes=info['delay_bytes'] if slot!=FX_SLOT else (validate(unpack(body)[2])[1]['delay_bytes'] if body else 0)
                if voice_bytes*2+fx_bytes>131072:raise ValueError(f'Synth slot {index+1} and slot 0 exceed the shared 128 KiB delay/reverb memory budget.')
        scratch=C.create_string_buffer(CAPACITY)
        rc=LIB.fms_slot_write(C.byref(self.flash),slot,payload,len(payload),expected,scratch)
        if rc==-2:raise ValueError('Slot changed since it was opened. Reload before saving.')
        if rc:raise ValueError('Save failed; previous committed copy remains available.')
    def knob_defaults(self,slot,info):
        other_slot=0 if slot==FX_SLOT else FX_SLOT
        other,_=self.get(other_slot)
        fallback=validate(unpack(other)[2],other_slot==FX_SLOT)[1]['controls'] if other else {}
        synth=fallback if slot==FX_SLOT else info['controls']
        effect=info['controls'] if slot==FX_SLOT else fallback
        return [max(0,min(1,synth.get('knob'+str(i),effect.get('knob'+str(i),.5)))) for i in range(1,5)]
    def describe(self,slot,full=False):
        payload,revision=self.get(slot);result=dict(slot=slot,revision=revision,used=len(payload),capacity=capacity(slot),role='effect' if slot==FX_SLOT else 'synth',empty=not payload)
        if payload:
            name,source,binary=unpack(payload);_,info=validate(binary,slot==FX_SLOT)
            result.update(name=name,info=info,has_source=bool(source),knob_defaults=self.knob_defaults(slot,info))
            if full:result.update(source=source,binary=base64.b64encode(binary).decode(),package=base64.b64encode(payload).decode())
        return result

def compile_source(source,effect=False):
    if not isinstance(source,str) or not 0<len(source.encode())<=CAPACITY:raise ValueError('Paste a SynthDef expression within the slot size limit.')
    if not SCLANG.exists():raise ValueError('Install SuperCollider to compile source, or import a .scsyndef file.')
    with tempfile.TemporaryDirectory(prefix='fm1-compile-') as tmp:
        tmp=Path(tmp);(tmp/'source.scd').write_text(source)
        script='var def;\ndef = File.readAllString('+json.dumps(str(tmp/'source.scd'))+').interpret;\nif(def.isKindOf(SynthDef).not, { "FM1_ERROR: Return one SynthDef expression".postln; 1.exit; });\nFile.use('+json.dumps(str(tmp/'patch.scsyndef'))+', "wb", { |f| f.write(def.asBytes); });\n"FM1_COMPILED".postln;\n0.exit;\n'
        (tmp/'compile.scd').write_text(script)
        try:run=subprocess.run(language_command(SCLANG,tmp/'compile.scd',tmp),capture_output=True,text=True,timeout=30)
        except subprocess.TimeoutExpired:raise ValueError('Compilation timed out. Check the source for syntax errors or non-terminating code.')
        if run.returncode or 'FM1_COMPILED' not in run.stdout or not (tmp/'patch.scsyndef').exists():raise ValueError((run.stdout+run.stderr)[-5000:])
        binary=(tmp/'patch.scsyndef').read_bytes();_,info=validate(binary,effect)
        return binary,info

def preview(binary,midi=60,velocity=90,knobs=None,effect_binary=None):
    if type(midi)!=int or not 0<=midi<=127 or type(velocity)!=int or not 1<=velocity<=127:raise ValueError('Invalid note or velocity.')
    graph,_=validate(binary);player=C.create_string_buffer(LIB.fms_performance_size())
    if not LIB.fms_init(player,graph):raise ValueError('Patch cannot start.')
    if effect_binary:
        effect_graph,_=validate(effect_binary,True);effect_voice=C.create_string_buffer(LIB.fms_voice_size())
        if not LIB.fms_attach_effect(player,effect_graph,effect_voice) or not LIB.fms_reset_effect(player):raise ValueError('Synth and effect exceed the shared delay/reverb memory budget.')
    if knobs is not None:
        if len(knobs)!=4 or not all(isinstance(x,(int,float)) and math.isfinite(x) and 0<=x<=1 for x in knobs):raise ValueError('Four knob values in 0–1 are required.')
        for i,x in enumerate(knobs):LIB.fms_knob(player,i,x)
    LIB.fms_note_on(player,1,midi,velocity);frames=array.array('h');meter=[]
    for block in range(2068 if effect_binary else 1379):
        if block==689:LIB.fms_note_off(player,1)
        buf=(C.c_float*128)();LIB.fms_render(player,buf)
        if block%20==0:meter.append([max(abs(buf[i]) for i in range(c,128,2)) for c in (0,1)])
        frames.extend(round(x*32767) for x in buf)
    if LIB.fms_faults(player):raise ValueError('Patch produced a numerical fault during preview.')
    if os.sys.byteorder!='little':frames.byteswap()
    output=io.BytesIO()
    with wave.open(output,'wb') as w:w.setparams((2,2,44100,0,'NONE','not compressed'));w.writeframes(frames.tobytes())
    return output.getvalue(),meter
