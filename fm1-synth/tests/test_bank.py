import base64
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
import struct
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from bank import Bank,pack,unpack,validate,preview,CAPACITY
FIXTURES=Path(__file__).resolve().parents[2]/'synthdef-engine/fixtures'

class BankTests(unittest.TestCase):
    def setUp(self):self.tmp=tempfile.TemporaryDirectory();self.path=Path(self.tmp.name)/'bank.flash';self.binary=(FIXTURES/'fm1_voice.scsyndef').read_bytes()
    def tearDown(self):self.tmp.cleanup()
    def test_binary_and_source_roundtrip(self):
        payload=pack('温かい声','// λ UTF-8\n',self.binary)
        self.assertEqual(unpack(payload),('温かい声','// λ UTF-8\n',self.binary))
        self.assertEqual(unpack(pack('Binary only','',self.binary))[1],'')
    def test_restart_delete_and_revision(self):
        bank=Bank(self.path);payload=pack('Voice','',self.binary);bank.put(0,payload,0)
        bank=Bank(self.path);self.assertEqual(bank.get(0),(payload,1))
        with self.assertRaisesRegex(ValueError,'changed'):bank.put(0,payload,0)
        bank.put(0,b'',1);self.assertEqual(Bank(self.path).get(0),(b'',2))
        self.assertEqual(bank.get(7),(b'',0))
    def test_container_rejections(self):
        payload=pack('Voice','// source',self.binary)
        for candidate in (b'',payload[:-1],payload+b'x',payload[:40]+bytes([payload[40]^1])+payload[41:]):
            with self.subTest(candidate=len(candidate)),self.assertRaises((ValueError,KeyError)):unpack(candidate)
        with self.assertRaisesRegex(ValueError,'capacity'):pack('Big','x'*CAPACITY,self.binary)
        with self.assertRaises(ValueError):pack('Bad\nname','',self.binary)
        with self.assertRaises(ValueError):validate(b'not a synthdef')
    def test_each_slot_independent(self):
        bank=Bank(self.path)
        for i in range(8):bank.put(i,pack('Voice '+str(i),'',self.binary),0)
        for i in range(8):self.assertEqual(bank.describe(i)['name'],'Voice '+str(i))
        with self.assertRaises(ValueError):bank.get(9)
        self.assertEqual(self.path.read_bytes()[:0x97000],b'\xff'*0x97000)
        self.assertEqual(self.path.read_bytes()[0xD7000:],b'\xff'*(0x100000-0xD7000))
    def test_preview_audio_and_velocity(self):
        audio,meters=preview(self.binary,69,100,[0,0.3,0.6,1])
        self.assertEqual(audio[:4],b'RIFF');self.assertGreater(max(max(m) for m in meters),0)
        _,quiet=preview(self.binary,69,10)
        self.assertLess(max(max(m) for m in quiet),max(max(m) for m in meters)/5)
        with self.assertRaises(ValueError):preview(self.binary,128,90)
    def test_effect_slot_and_shared_delay_budget(self):
        examples=FIXTURES.parents[1]/'fm1-synth/examples'
        fx=(examples/'master-echo.scsyndef').read_bytes()
        effect=pack('Echo','',fx,True);bank=Bank(self.path)
        bank.put(8,effect,0);self.assertEqual(bank.describe(8)['capacity'],7936)
        self.assertEqual(Bank(self.path).get(8),(effect,1))
        # Increase the existing Keys delay maximum from 0.1 to 0.2 seconds.
        large=bytearray((examples/'keys.scsyndef').read_bytes());offset=11+large[10]
        count=struct.unpack_from('>i',large,offset)[0];offset+=4
        for i in range(count):
            if abs(struct.unpack_from('>f',large,offset+4*i)[0]-.1)<1e-7:struct.pack_into('>f',large,offset+4*i,.2)
        _,info=validate(bytes(large));self.assertEqual(info['delay_bytes'],65536)
        candidate=pack('Large delay','',bytes(large))
        with self.assertRaisesRegex(ValueError,'shared'):bank.put(0,candidate,0)
        bank.put(8,b'',1);bank.put(0,candidate,0)
        with self.assertRaisesRegex(ValueError,'shared'):bank.put(8,effect,2)
        self.assertEqual(bank.get(8),(b'',2))
        self.assertEqual(self.path.read_bytes()[0xDB000:],b'\xff'*(0x100000-0xDB000))
        with self.assertRaises(ValueError):pack('Too large','x'*7936,fx,True)
        with self.assertRaises(ValueError):validate(fx)

if __name__=='__main__':unittest.main()
