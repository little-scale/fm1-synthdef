#!/usr/bin/env python3
"""Preserve existing effects, move legacy controls to K5–K8, fill empty FX2–FX4.

Stores original effect packages in --backup before any writes. Synth slots are
never written. Device transfers require the new four-effect firmware.
"""
import argparse,re,struct
from pathlib import Path
from bank import ROOT,Bank,pack,unpack

def remap(payload):
    if not payload:return payload
    name,source,binary=unpack(payload);b=bytearray(binary)
    # Walk the version-2 SynthDef parameter-name table; do not replace arbitrary
    # byte patterns in constants or UGen data. Renaming preserves DSP bytecode.
    assert b[:4]==b'SCgf' and struct.unpack_from('>i',b,4)[0]==2 and struct.unpack_from('>h',b,8)[0]==1
    at=11+b[10]
    for _ in range(2):
        count=struct.unpack_from('>i',b,at)[0];at+=4+count*4
    count=struct.unpack_from('>i',b,at)[0];at+=4;entries=[]
    for _ in range(count):
        size=b[at];at+=1;control=bytes(b[at:at+size]).decode();entries.append((at,size,control));at+=size+4
    names={control for _,_,control in entries};old=sorted(names.intersection({'knob1','knob2','knob3','knob4'}))
    if not old:return payload
    mapping={control:'knob'+str(5+i) for i,control in enumerate(old)}
    if names.intersection(mapping.values()):raise ValueError('Mixed old/new effect controls need manual renaming; original preserved')
    for at,size,control in entries:
        if control in mapping:b[at:at+size]=mapping[control].encode()
    source=re.sub(r'\bknob([1-4])\b',lambda m:mapping.get(m[0],m[0]),source)
    return pack(name,source,bytes(b),True)

def migrate(get,put,backup):
    backup.mkdir(parents=True,exist_ok=True);original={i:get(i)[0] for i in range(8,12)}
    # Complete the reviewable plan and save originals before changing storage.
    planned={i:remap(data) for i,data in original.items()}
    for i,name in [(9,'master-freeverb'),(10,'master-freeverb2'),(11,'master-miverb')]:
        if not planned[i]:planned[i]=(ROOT/'examples'/(name+'.fmslot')).read_bytes()
    if sum(map(len,planned.values()))>7916:raise ValueError('Effects exceed the shared 7,916-byte budget; originals unchanged')
    for i,data in original.items():
        path=backup/f'fx{i-7}.fmslot'
        if path.exists() and path.read_bytes()!=data:raise ValueError('Backup already differs; use a fresh backup directory')
        path.write_bytes(data)
    for i,data in planned.items():
        old,revision=get(i)
        if old!=original[i]:raise ValueError('Effect changed during migration; stopping without overwriting it')
        if old!=data:put(i,data,revision)
        assert get(i)[0]==data
    return {f'FX{i-7}':len(data) for i,data in planned.items()}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--device',action='store_true');p.add_argument('--bank',type=Path,default=ROOT/'data/local-bank.flash');p.add_argument('--backup',type=Path,required=True);a=p.parse_args()
    if a.device:
        from device import Device
        d=Device()
        try:
            if d.info[:2]!=bytes([3,12]):raise ValueError('Install the four-effect firmware first')
            print(migrate(d.read,d.write,a.backup))
        finally:d.close()
    else:
        bank=Bank(a.bank);print(migrate(bank.get,bank.put,a.backup))
