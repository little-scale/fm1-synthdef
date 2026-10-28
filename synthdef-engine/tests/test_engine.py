#!/usr/bin/env python3
"""API/hostile-input tests; only Python's standard library and a C compiler."""
import ctypes as C
import math
from pathlib import Path
import random
import struct
import subprocess
import unittest
ROOT = Path(__file__).resolve().parents[1]
subprocess.run(['cc','-std=c99','-O2','-Wall','-Wextra','-Werror','-fno-fast-math','-ffp-contract=off','-shared','-fPIC','-Isrc','src/fm_load.c','src/fm_render.c','tests/bridge.c','-lm','-o','build/test-engine.dylib'],cwd=ROOT,check=True)
lib=C.CDLL(str(ROOT/'build/test-engine.dylib'))
for name in ('graph','voice','scratch'):
    getattr(lib,f'test_{name}_size').restype=C.c_size_t
lib.fm_load.argtypes=[C.c_void_p,C.c_void_p,C.c_size_t,C.c_char_p,C.c_size_t]
lib.fm_start.argtypes=[C.c_void_p,C.c_void_p,C.c_float]
lib.fm_seed.argtypes=[C.c_void_p,C.c_uint32]
lib.fm_bind_delay.argtypes=[C.c_void_p,C.c_void_p,C.c_void_p,C.c_uint32]
lib.fm_delay_required.argtypes=[C.c_void_p,C.c_float]
lib.fm_delay_required.restype=C.c_uint32
lib.fm_set.argtypes=[C.c_void_p,C.c_void_p,C.c_char_p,C.c_float]
lib.fm_render.argtypes=[C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p]
lib.fm_render_input.argtypes=[C.c_void_p]*5
lib.fm_prepare.argtypes=[C.c_void_p,C.c_void_p,C.c_void_p]
lib.test_active.argtypes=lib.test_fault.argtypes=[C.c_void_p]
def ps(s):
    s=s.encode();return bytes([len(s)])+s
def i(n):return struct.pack('>i',n)
def f(n):return struct.pack('>f',n)
def synth(nodes, constants=(0,1), controls=(), names=(), variants=0):
    out=b'SCgf'+i(2)+struct.pack('>h',1)+ps('test')+i(len(constants))+b''.join(map(f,constants))
    out+=i(len(controls))+b''.join(map(f,controls))+i(len(names))
    out+=b''.join(ps(name)+i(index) for name,index in names)+i(len(nodes))
    for name,rate,inputs,outputs,special in nodes:
        out+=ps(name)+bytes([rate])+i(len(inputs))+i(len(outputs))+struct.pack('>h',special)
        out+=b''.join(i(a)+i(b) for a,b in inputs)+bytes(outputs)
    return out+struct.pack('>h',variants)
def node(name,inputs=(),rate=2,outputs=None,special=0):
    return (name,rate,inputs,[rate] if outputs is None else outputs,special)
class Engine:
    def __init__(self,data):
        self.g=C.create_string_buffer(lib.test_graph_size())
        self.v=C.create_string_buffer(lib.test_voice_size())
        self.b=C.create_string_buffer(lib.test_scratch_size())
        self.delay=(C.c_float*65536)()
        err=C.create_string_buffer(256)
        self.ok=lib.fm_load(self.g,data,len(data),err,256)
        self.error=err.value.decode(errors='replace')
    def start(self,sr=44100):return lib.fm_start(self.v,self.g,sr) and lib.fm_bind_delay(self.v,self.g,self.delay,65536)
    def set(self,key,value):return lib.fm_set(self.v,self.g,key.encode(),value)
    def render(self,seed=0):
        out=(C.c_float*128)(*[seed]*128);lib.fm_render(self.g,self.v,self.b,out);return list(out)
class Tests(unittest.TestCase):
    def test_stereo_input_bus(self):
        data=synth([node('In',[(-1,0)],outputs=[2,2]),node('Out',[(-1,0),(0,0),(0,1)],outputs=[])])
        e=Engine(data);self.assertTrue(e.ok,e.error);self.assertTrue(e.start())
        inp=(C.c_float*128)(*[.125 if j%2==0 else -.25 for j in range(128)])
        out=(C.c_float*128)();lib.fm_render_input(e.g,e.v,e.b,out,inp)
        self.assertEqual(list(out),list(inp));self.assertEqual(e.render(),[0]*128)
        # A dynamic nonzero bus is a voice fault, never out-of-bounds access.
        data=synth([node('Control',rate=1),node('In',[(0,0)],outputs=[2,2]),node('Out',[(-1,0),(1,0),(1,1)],outputs=[])],controls=(0,),names=(('bus',0),))
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();e.set('bus',1)
        out=(C.c_float*128)();lib.fm_render_input(e.g,e.v,e.b,out,inp)
        self.assertTrue(lib.test_fault(e.v));self.assertEqual(list(out),[0]*128)
        for n in (node('In',[(-1,1)],outputs=[2,2]),node('In',[(-1,0)],rate=1),node('In',[(-1,0)],outputs=[2,2,2])):
            self.assertFalse(Engine(synth([n])).ok)

    def test_preparation_preserves_audio_and_shared_scratch(self):
        # Preparation is silent/idempotent and must not depend on scratch
        # retained from that constructor once another voice has rendered.
        for name in ('perc','m5_addy','m5_random_noise','m4_CombC_audio'):
            data=(ROOT/'fixtures'/f'{name}.scsyndef').read_bytes()
            normal,prepared,other=Engine(data),Engine(data),Engine(data)
            for e in (normal,prepared,other):self.assertTrue(e.ok,e.error);self.assertTrue(e.start())
            lib.fm_prepare(prepared.g,prepared.v,prepared.b)
            lib.fm_prepare(prepared.g,prepared.v,prepared.b)
            self.assertEqual(lib.fm_seed(prepared.v,123),0)
            other.b=prepared.b
            for block in range(40):
                other.render()
                self.assertEqual(normal.render(),prepared.render(),(name,block))
            self.assertFalse(lib.test_fault(prepared.v))
    def test_numeric_curve_generalization(self):
        # Independent analytic oracle, especially increasing positive curvature with nonzero starts.
        for curve in (-20,-12,-4,-0.01,0.01,4,12,20,40):
            for start,end in ((0,1),(1,0),(-1,1),(2,3)):
                constants=(0,1,start,end,-99,5,curve)
                refs=[(-1,k) for k in (1,1,0,1,0,2,1,4,4,3,1,5,6)]
                e=Engine(synth([node('EnvGen',refs),node('Out',[(-1,0),(0,0)],outputs=[])],constants=constants))
                self.assertTrue(e.ok,e.error);e.start();peak=0
                c=struct.unpack('f',struct.pack('f',curve))[0]
                for block in range(690):
                    values=e.render()[::2]
                    for j,value in enumerate(values):
                        t=min((block*64+j)/44100,1)
                        expected=start+(end-start)*math.expm1(c*t)/math.expm1(c)
                        peak=max(peak,abs(value-expected))
                self.assertLess(peak,0.00001 if abs(curve)>20 else 0.000005,(curve,start,end,peak));self.assertFalse(lib.test_fault(e.v))
    def test_random_initialization_and_shared_scratch(self):
        for kind in ('Rand','ExpRand'):
            data=synth([node('Control',rate=1),node(kind,[(-1,1),(0,0)],rate=0),node('DC',[(1,0)]),node('Out',[(-1,0),(2,0)],outputs=[])],constants=(0,0.1),controls=(0.9,),names=(('hi',0),))
            a,b,c=Engine(data),Engine(data),Engine(data)
            for e in (a,b,c):self.assertTrue(e.ok,e.error);self.assertTrue(e.start())
            lib.fm_seed(a.v,123);lib.fm_seed(b.v,123);lib.fm_seed(c.v,124)
            first=a.render();self.assertEqual(first,b.render());self.assertNotEqual(first,c.render())
            self.assertTrue(all(0.1<=x<=0.9 for x in first[::2]))
            c.b=a.b;a.set('hi',5)
            for _ in range(20):c.render();self.assertEqual(a.render(),first)
            a.start();lib.fm_seed(a.v,123);self.assertEqual(a.render(),first)
    def test_random_domains_and_rates(self):
        for lo,hi in ((0,1),(1,0),(-1,1),(1,-1)):
            data=synth([node('ExpRand',[(-1,1),(-1,2)],rate=0),node('DC',[(0,0)]),node('Out',[(-1,0),(1,0)],outputs=[])],constants=(0,lo,hi))
            e=Engine(data);self.assertTrue(e.ok,e.error);e.start()
            self.assertEqual(e.render(0.25),[0.25]*128);self.assertTrue(lib.test_fault(e.v))
        for kind in ('Rand','ExpRand'):
            for rate in (1,2):self.assertFalse(Engine(synth([node(kind,[(-1,0),(-1,1)],rate=rate)])).ok)
            self.assertFalse(Engine(synth([node('DC',[(-1,1)]),node(kind,[(-1,0),(0,0)],rate=0)])).ok)
            self.assertFalse(Engine(synth([node(kind,[(-1,0)],rate=0)])).ok)
    def test_output_bus_control_and_fault_isolation(self):
        data=synth([node('Control',rate=1),node('DC',[(-1,1)]),node('Out',[(0,0),(1,0),(1,0)],outputs=[])],controls=(0,),names=(('output',0),))
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();self.assertEqual(e.render(),[1]*128)
        for bus in (1,2,-1,0.5):
            e.start();e.set('output',bus);self.assertEqual(e.render(0.25),[0.25]*128);self.assertTrue(lib.test_fault(e.v))
        e.start();e.render();e.set('output',1);self.assertEqual(e.render(0.25),[0.25]*128)
        self.assertFalse(Engine(synth([node('DC',[(-1,1)]),node('Out',[(-1,1),(0,0)],outputs=[])])).ok)
        self.assertFalse(Engine(synth([node('DC',[(-1,0)]),node('Out',[(0,0),(0,0)],outputs=[])])).ok)
    def test_enlarged_graph_boundaries(self):
        for count,valid in ((160,True),(161,False)):
            e=Engine(synth([node('DC',[(-1,1)])]*count));self.assertEqual(bool(e.ok),valid,e.error)
            if valid:self.assertTrue(e.start());e.render();self.assertFalse(lib.test_fault(e.v))
        for count,valid in ((128,True),(129,False)):
            e=Engine(synth([node('Control',rate=1,outputs=[1]*64)]+[node('DC',[(-1,1)])]*count,controls=(0,)*64))
            self.assertEqual(bool(e.ok),valid,e.error)
    def test_addy_lifetime(self):
        e=Engine((ROOT/'fixtures/m5_addy.scsyndef').read_bytes());self.assertTrue(e.ok,e.error);e.start()
        peak=0
        for _ in range(725):peak=max(peak,max(abs(x) for x in e.render()))
        self.assertGreater(peak,0.001);self.assertFalse(lib.test_fault(e.v));self.assertFalse(lib.test_active(e.v))
        self.assertEqual(e.render(),[0]*128)
    def test_delay_memory_contract(self):
        e=Engine((ROOT/'fixtures/m4_CombC_ar_fixed.scsyndef').read_bytes())
        need=lib.fm_delay_required(e.g,44100);self.assertEqual(need,2048)
        self.assertTrue(lib.fm_start(e.v,e.g,44100));self.assertFalse(lib.fm_bind_delay(e.v,e.g,e.delay,need-1))
        self.assertEqual(e.render(0.25),[0.25]*128);self.assertTrue(lib.test_fault(e.v))
        self.assertTrue(e.start());first=[e.render() for _ in range(8)]
        self.assertFalse(lib.fm_bind_delay(e.v,e.g,e.delay,65536))
        self.assertTrue(e.start());self.assertEqual([e.render() for _ in range(8)],first)
    def test_delay_limits_and_rejections(self):
        too_big=synth([node('DC',[(-1,0)]),node('CombC',[(0,0),(-1,1),(-1,0),(-1,1)])],constants=(0,100))
        e=Engine(too_big);self.assertTrue(e.ok);self.assertEqual(lib.fm_delay_required(e.g,44100),0xffffffff);self.assertFalse(e.start())
        for kind in ('DelayN','DelayL','DelayC','CombN','CombL','CombC','AllpassN','AllpassL','AllpassC'):
            refs=[(0,0),(-1,1),(-1,0)]+([(-1,1)] if not kind.startswith('Delay') else [])
            self.assertFalse(Engine(synth([node('DC',[(-1,0)]),node(kind,refs)],constants=(0,-1))).ok)
            refs[1]=(0,0)
            self.assertFalse(Engine(synth([node('DC',[(-1,0)]),node(kind,refs)])).ok)
    def test_delay_impulse_and_negative_feedback(self):
        # Independent integer-delay oracle: first echo 4 samples later, alternating signs.
        data=synth([node('Control',[],rate=1,outputs=[1]),node('K2A',[(0,0)]),node('CombN',[(1,0),(-1,1),(-1,2),(-1,3)]),node('Out',[(-1,0),(2,0)],outputs=[])],constants=(0,0.001,4/44100,-0.001),controls=(1,),names=(('x',0),))
        e=Engine(data);self.assertTrue(e.start());out=e.render();self.assertEqual(out[:8],[0]*8);self.assertAlmostEqual(out[8],1,places=6)
        feedback=-math.exp(-math.log(1000)*(4/44100)/0.001)
        self.assertAlmostEqual(out[16],1+feedback,places=6)
    def test_exported_fixtures(self):
        files=list((ROOT/'fixtures').glob('*.scsyndef'));self.assertGreaterEqual(len(files),30)
        for path in files:
            with self.subTest(path=path.name):
                e=Engine(path.read_bytes());self.assertTrue(e.ok,e.error);self.assertTrue(e.start())
                for _ in range(350):self.assertTrue(all(math.isfinite(x) for x in e.render()))
                self.assertFalse(lib.test_fault(e.v))
    def test_every_truncation_and_trailing_bytes(self):
        data=(ROOT/'fixtures/perc.scsyndef').read_bytes()
        for end in range(len(data)):
            self.assertFalse(Engine(data[:end]).ok,end)
        self.assertFalse(Engine(data+b'\0').ok)
    def test_rejections(self):
        cases=[
            synth([node('Unknown')]), synth([node('SinOsc',[(-1,0),(-1,0)],rate=3)]),
            synth([node('DC',[(-1,99)])]),synth([node('DC',[(0,0)])]),
            synth([node('DC',[(-2,0)])]),synth([node('DC',[(-1,0)],outputs=[1])]),
            synth([node('DC',[(-1,0)],special=1)]),synth([node('DC',[])]),
            synth([node('DC',[(-1,0)])],constants=(math.nan,)),
            synth([node('Control',[],rate=1,outputs=[1],special=1)],controls=(1,)),
            synth([node('Control',[],rate=1,outputs=[1])],controls=(1,),names=(('x',1),)),
            synth([node('Control',[],rate=1,outputs=[1])],controls=(1,),names=(('x',0),('x',0))),
            synth([node('DC',[(-1,0)])],variants=1),
            synth([node('BinaryOpUGen',[(-1,0),(-1,1)],special=99)]),
            synth([node('UnaryOpUGen',[(-1,0)],special=99)]),
            synth([node('Out',[(-1,0),(-1,1)],outputs=[])]),
            synth([node('DC',[(-1,1)]),node('Resonz',[(0,0),(0,0),(-1,1)])]),
        ]
        for data in cases:
            e=Engine(data);self.assertFalse(e.ok,e.error);self.assertFalse(e.start())
        for offset in (10+len('test')+1,):
            data=bytearray(synth([node('DC',[(-1,0)])]));data[offset:offset+4]=i(0x7fffffff)
            self.assertFalse(Engine(bytes(data)).ok)
        self.assertFalse(Engine(b'X'*65537).ok)
    def test_failed_load_invalidates_graph(self):
        e=Engine((ROOT/'fixtures/tone.scsyndef').read_bytes());self.assertTrue(e.start())
        err=C.create_string_buffer(256)
        self.assertFalse(lib.fm_load(e.g,b'bad',3,err,256));self.assertFalse(e.start())
    def test_control_and_restart(self):
        e=Engine((ROOT/'fixtures/tone.scsyndef').read_bytes());self.assertTrue(e.start())
        a=e.render();self.assertGreater(max(a),0.06)
        self.assertTrue(e.set('amp',0));self.assertGreater(max(abs(x) for x in e.render()),0);self.assertEqual(e.render(),[0]*128)
        self.assertFalse(e.set('missing',1));self.assertFalse(e.set('amp',math.inf))
        self.assertTrue(e.start());self.assertEqual(e.render(),a)
        for sr in (0,7999,192001,math.nan,math.inf):self.assertFalse(e.start(sr))
    def test_scalar_controls_freeze(self):
        data=synth([node('Control',[],rate=0,outputs=[0]),node('DC',[(0,0)]),node('Out',[(-1,0),(1,0)],outputs=[])],controls=(0.2,),names=(('x',0),))
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();self.assertTrue(e.set('x',0.4));out=e.render()
        self.assertAlmostEqual(out[0],0.4);self.assertFalse(e.set('x',0.8))
    def test_additive_output_and_scratch_sharing(self):
        data=synth([node('DC',[(-1,1)]),node('Out',[(-1,0),(0,0)],outputs=[]),node('Out',[(-1,0),(0,0),(0,0)],outputs=[])])
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start()
        self.assertEqual(e.render(0.25),[2.25,1.25]*64)
        a=Engine((ROOT/'fixtures/tone.scsyndef').read_bytes());b=Engine((ROOT/'fixtures/perc.scsyndef').read_bytes());c=Engine((ROOT/'fixtures/tone.scsyndef').read_bytes())
        a.start();b.start();c.start();b.b=a.b
        for _ in range(25):b.render();self.assertEqual(a.render(),c.render())
    def test_done_action_stops_and_restart_resets(self):
        e=Engine((ROOT/'fixtures/perc.scsyndef').read_bytes());e.start()
        for _ in range(200):e.render()
        self.assertFalse(lib.test_active(e.v));self.assertEqual(e.render(),[0]*128)
        e.start();self.assertTrue(lib.test_active(e.v));self.assertGreater(max(e.render()),0)
    def test_numeric_fault(self):
        data=synth([node('DC',[(-1,1)]),node('BinaryOpUGen',[(0,0),(-1,0)],special=4),node('Out',[(-1,0),(1,0)],outputs=[])])
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();out=e.render()
        self.assertTrue(lib.test_fault(e.v));self.assertFalse(lib.test_active(e.v));self.assertTrue(all(math.isfinite(x) for x in out))
    def test_trigger_control_one_block_and_retrigger(self):
        data=synth([node('TrigControl',[],rate=1),node('DC',[(-1,1)]),node('BinaryOpUGen',[(1,0),(0,0)],special=2),node('Out',[(-1,0),(2,0)],outputs=[])],controls=(0,),names=(('gate',0),))
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();self.assertEqual(e.render(),[0]*128)
        e.set('gate',1);up=e.render()[::2];down=e.render()[::2]
        for i in range(64):self.assertAlmostEqual(up[i],i/64,places=6);self.assertAlmostEqual(down[i],1-i/64,places=6)
        self.assertEqual(e.render(),[0]*128)
        e.set('gate',1);self.assertEqual(e.render()[::2],up)
    def test_new_layout_rejections(self):
        cases=[
            synth([node('LagControl',[],rate=1)],controls=(0,)),
            synth([node('TrigControl',[],rate=2)],controls=(0,)),
            synth([node('WhiteNoise',[(-1,0)])]),
            synth([node('RandSeed',[(-1,1),(-1,2)],rate=0)],constants=(0,1,0.5)),
            synth([node('Select',[(-1,0),(-1,1)])]),
            synth([node('Control',[],rate=1),node('LFSaw',[(-1,1),(0,0)])],controls=(0,)),
        ]
        for data in cases:self.assertFalse(Engine(data).ok)
    def test_oscillator_invalid_domain_faults_silently(self):
        for hz in (0,-1,1e-30,1e30):
            e=Engine(synth([node('Saw',[(-1,1)]),node('Out',[(-1,0),(0,0)],outputs=[])],constants=(0,hz)))
            self.assertTrue(e.ok,e.error);e.start();self.assertEqual(e.render(0.25),[0.25]*128);self.assertTrue(lib.test_fault(e.v))
    def test_noise_seed_and_voice_independence(self):
        data=synth([node('WhiteNoise'),node('Out',[(-1,0),(0,0)],outputs=[])])
        a,b,c=Engine(data),Engine(data),Engine(data)
        for e in (a,b,c):self.assertTrue(e.ok,e.error);e.start()
        self.assertTrue(lib.fm_seed(a.v,123));self.assertTrue(lib.fm_seed(b.v,123));self.assertTrue(lib.fm_seed(c.v,124))
        first=a.render();self.assertEqual(first,b.render());self.assertNotEqual(first,c.render());self.assertFalse(lib.fm_seed(a.v,5))
        c.b=a.b
        for _ in range(10):c.render();self.assertEqual(a.render(),b.render())
    def test_modulated_resonator_import(self):
        data=synth([node('Control',[],rate=1),node('DC',[(-1,0)]),node('Resonz',[(1,0),(0,0),(-1,1)])],controls=(440,))
        e=Engine(data);self.assertTrue(e.ok,e.error);e.start();e.render();self.assertFalse(lib.test_fault(e.v))
    def test_random_mutations_remain_bounded(self):
        rng=random.Random(29401);original=(ROOT/'fixtures/perc.scsyndef').read_bytes()
        for _ in range(1500):
            data=bytearray(original)
            for _ in range(rng.randrange(1,5)):data[rng.randrange(len(data))]=rng.randrange(256)
            e=Engine(bytes(data))
            if e.ok:
                e.start()
                for _ in range(4):self.assertTrue(all(math.isfinite(x) for x in e.render()))
if __name__=='__main__':unittest.main(verbosity=2)
