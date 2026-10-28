#!/usr/bin/env python3
"""Exercise slot 3's two voices and four controls with the slot 0 echo active."""
import argparse, array, json, subprocess, time, wave
from pathlib import Path
import mido
from device import Device

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--output', type=Path, required=True)
out = p.parse_args().output
out.mkdir(parents=True, exist_ok=True)
dev = Device()
samples = []
capture = None

def remote(data):
    dev.out.send(mido.Message('sysex', data=[0x7d, 0x4c, 0x43] + data))
    time.sleep(.08)

def home():
    remote([1, 64, 0]); remote([1, 0, 0])

def select(index):
    state = dev.stats()
    remote([3, 2, 64 + index - state['slot_index']])

def notes(keys, on):
    for key in keys:
        dev.out.send(mido.Message('note_on' if on else 'note_off',
                                  note=key, velocity=80 if on else 0))

def check(label, voices):
    state = dev.stats()
    state['phase'] = label
    samples.append(state)
    print(json.dumps(state), flush=True)
    assert state['slot_index'] == 2 and state['loaded'] and state['effect_active']
    assert state['active_voices'] == voices and not state['faults']
    assert state['xruns'] == baseline['xruns']
    assert state['overloads'] == baseline['overloads']

try:
    if dev.panel()['fx_mode']:remote([1,1,0]);remote([1,0,0])
    remote([3,1,65-dev.panel()['fx_choice']])
    remote([4, 0, 8]); select(0); select(2); home()
    baseline = dev.stats()
    with (out / 'capture.log').open('w') as log:
        capture = subprocess.Popen([
            'ffmpeg', '-hide_banner', '-nostdin', '-f', 'avfoundation',
            '-i', ':SynthDef FM-1', '-t', '22', '-ar', '44100', '-ac', '2',
            '-c:a', 'pcm_s16le', '-y', str(out / 'triple-saw-device.wav')
        ], stdout=log, stderr=subprocess.STDOUT)
        time.sleep(2)
        notes((60, 67), True); time.sleep(.4); check('default chord', 2)
        for knob in range(4):
            for delta in (-50, 50):
                remote([3, 3 + knob, 64 + delta])
                check('held chord knob %d %s' % (knob + 1, 'min' if delta < 0 else 'max'), 2)
        notes((60, 67), False)
        time.sleep(.25); check('long release remains active', 2)
        time.sleep(3.1); check('long release complete', 0)
        for key in (36, 48, 72, 96):
            home(); notes((key, key + 7), True)
            time.sleep(1.7); check('maximum controls chord %d' % key, 2)
            notes((key, key + 7), False)
        home()
        for knob in range(4):
            remote([3, 3 + knob, 14])
        notes((60, 67), True); time.sleep(.3); check('minimum controls chord', 2)
        notes((60, 67), False); time.sleep(.2); check('short release complete', 0)
        capture.wait(timeout=30)
        assert capture.returncode == 0, 'USB recording failed'
finally:
    home(); select(0); select(2); home(); remote([4, 1, 32])
    dev.close()
    if capture and capture.poll() is None:
        capture.terminate()
    (out / 'performance.json').write_text(json.dumps(samples, indent=2) + '\n')

with wave.open(str(out / 'triple-saw-device.wav')) as wav:
    assert wav.getnchannels() == 2
    pcm = array.array('h', wav.readframes(wav.getnframes()))
peaks = [max(abs(x) for x in pcm[channel::2]) / 32768 for channel in range(2)]
assert min(peaks) > .0001 and max(peaks) < .99
result = dict(checks=len(samples), stereo_peaks=peaks,
              new_overloads=samples[-1]['overloads']-baseline['overloads'],
              new_xruns=samples[-1]['xruns']-baseline['xruns'],
              sampled_render_max_us=max(s['render_last_us'] for s in samples))
(out / 'verification.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
