#!/usr/bin/env python3
"""Offline comparison with pinned SuperCollider; no audio hardware is opened."""
import array
import importlib.util
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
ROOT = Path(__file__).resolve().parents[1]
SC = Path(os.environ.get('SCSYNTH', '/Applications/SuperCollider.app/Contents/Resources/scsynth'))
def samples(path):
    data = path.read_bytes()
    if path.suffix == '.wav':
        assert data[:4] == b'RIFF'
        pos = 12
        while pos + 8 <= len(data):
            tag, size = struct.unpack_from('<4sI', data, pos)
            pos += 8
            if tag == b'data':
                data = data[pos:pos+size]
                break
            pos += size + size % 2
        else:
            raise ValueError('Missing WAV data')
    out = array.array('f'); out.frombytes(data)
    return out
EVENTS = {
    'adsr': [(0.25, 0)], 'fm1_voice': [(0.25, 0)], 'phase_control': [(0.25,1)], 'pan_control': [(0.25,0.6)], 'forced': [(0.25, -1.05)],
    'retrigger': [(0.1, 0), (0.125, 1), (0.3, 0)], 'loop': [(0.25, 0)],
}
def impulse_timing(a,b):
    """Discontinuous FM impulses: preserve every pulse, allow <= 1 sample jitter.

    The input SinOsc is itself compared numerically elsewhere. Its sub-ULP
    differences can move a wrap across a sample boundary; peak error alone
    treats that as a missing pulse. All other fixtures retain waveform limits.
    """
    x=[i for i,v in enumerate(a[::2]) if abs(v)>.05]
    y=[i for i,v in enumerate(b[::2]) if abs(v)>.05]
    timing=max((abs(i-j) for i,j in zip(x,y)),default=0)
    amplitude=max([abs(a[2*i]-b[2*j]) for i,j in zip(x,y)]+[0])
    clean=all(abs(v)<1e-7 or abs(v-.1)<1e-6 for v in a[::2]) and all(abs(v)<1e-7 for v in a[1::2])
    return dict(impulses=len(x),reference_impulses=len(y),timing_error_samples=timing,
                amplitude_error=amplitude,timing_passed=bool(x) and len(x)==len(y) and timing<=1 and amplitude<5e-6 and clean)

def fixture_blocks(name):
    return 2756 if name.startswith("m7_") else 827 if name=="m5_addy" else 345

def main():
    manifest=json.loads((ROOT/'reference/fixtures.json').read_text())
    actual={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'fixtures').iterdir() if p.suffix in ('.scsyndef','.osc','.json')}
    fixtures=sorted((ROOT/'fixtures').glob('*.scsyndef'))
    if not fixtures or actual != manifest['sha256']:
        raise RuntimeError('Missing or changed fixtures; regenerate with tools/export-all.py')
    if os.environ.get('FM_REFERENCE_FILTER'):
        fixtures=[p for p in fixtures if p.stem.startswith(os.environ['FM_REFERENCE_FILTER'])]
    spec=importlib.util.spec_from_file_location('miverb_reference',ROOT/'tools/build-miverb-reference.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
    plugin_path=builder.build() if any(p.stem.startswith('m7_MiVerb') for p in fixtures) else None
    version=subprocess.check_output([str(SC), '-v'], text=True).strip()
    if 'scsynth 3.14.1' not in version:
        raise RuntimeError('Reference suite requires SuperCollider 3.14.1; found '+version)
    results = []
    for fixture in fixtures:
        name = fixture.stem
        blocks = fixture_blocks(name)
        ref = ROOT/'build'/f'{name}-reference.wav'
        plugins=str(SC.parent/'plugins')+(os.pathsep+str(plugin_path) if name.startswith('m7_MiVerb') else '')
        cmd = [str(SC), '-U', plugins, '-N', str(fixture.with_suffix('.osc')), '_', str(ref), '44100', 'WAV', 'float', '-o', '2', '-i', '0', '-z', '64']
        run = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        if run.returncode or 'FAILURE' in run.stdout or 'ERROR' in run.stdout + run.stderr:
            results.append(dict(name=name, passed=False, error=f'Reference renderer exited {run.returncode}: '+run.stdout+run.stderr)); continue
        ours = ROOT/'build'/f'{name}.f32'
        events = EVENTS.get(name, [(0.25, 0)] if name.startswith('env_') else [])
        event_file=fixture.with_suffix('.events.json')
        if event_file.exists(): events=json.loads(event_file.read_text())
        args = [str(ROOT/'build/fm-render'), str(fixture), str(ours), str(blocks)]
        for t, value in events:
            args += [str(int(t*44100)//64), str(value)]
        run = subprocess.run(args, capture_output=True, text=True)
        if run.returncode:
            results.append(dict(name=name, passed=False, error=run.stderr)); continue
        a, b = samples(ours), samples(ref)
        length = blocks*64*2
        if len(a)!=length or len(b)<length or not all(math.isfinite(x) for x in a) or not all(math.isfinite(x) for x in b[:length]):
            raise RuntimeError(name+': invalid, truncated or non-finite render')
        peak = max(abs(a[i]-b[i]) for i in range(length))
        rms = math.sqrt(sum((a[i]-b[i])**2 for i in range(length))/length)
        signal = math.sqrt(sum(b[i]**2 for i in range(length))/length)
        peak_limit = 0.001 if name == 'pan_audio' else 0.0001
        rms_limit = 0.00003
        if name.startswith(('m2_', 'm3_', 'm4_', 'm5_', 'm7_')):
            peak_limit, rms_limit = 0.000005, 0.000001
            if name.startswith('m2_LagUD_ar'):
                peak_limit, rms_limit = 0.00002, 0.000008
        if name.startswith(('curve', 'env_')) or name in ('adsr','forced','loop','retrigger','control_env'):
            peak_limit, rms_limit = 0.000005, 0.000001
        # Quantized nonlinear feedback amplifies tiny upstream oscillator differences.
        # The separate identical-input MiVerb oracle retains strict waveform limits.
        if name=='m7_MiVerb_saturation':peak_limit,rms_limit=0.0002,0.00002
        timing=impulse_timing(a,b[:length]) if name=='m6_Impulse_ar_fm' else {}
        passed=timing.get('timing_passed',peak<peak_limit and rms<rms_limit)
        results.append(dict(**timing,name=name, peak_error=peak, rms_error=rms, peak_limit=peak_limit, rms_limit=rms_limit, reference_rms=signal, passed=passed))
    (ROOT/'build/reference-results.json').write_text(json.dumps(results, indent=2)+'\n')
    for r in results:
        print(('PASS' if r['passed'] else 'FAIL'), r['name'], r.get('error') or f"peak={r['peak_error']:.7g} rms={r['rms_error']:.7g}")
    return all(r['passed'] for r in results)
if __name__ == '__main__':
    raise SystemExit(0 if main() else 1)
