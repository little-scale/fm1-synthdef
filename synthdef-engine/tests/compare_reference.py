#!/usr/bin/env python3
"""Offline comparison with pinned SuperCollider; no audio hardware is opened."""
import array
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
def main():
    manifest=json.loads((ROOT/'reference/fixtures.json').read_text())
    actual={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'fixtures').iterdir() if p.suffix in ('.scsyndef','.osc','.json')}
    fixtures=sorted((ROOT/'fixtures').glob('*.scsyndef'))
    if not fixtures or actual != manifest['sha256']:
        raise RuntimeError('Missing or changed fixtures; regenerate with tools/export-all.py')
    version=subprocess.check_output([str(SC), '-v'], text=True).strip()
    if 'scsynth 3.14.1' not in version:
        raise RuntimeError('Reference suite requires SuperCollider 3.14.1; found '+version)
    results = []
    for fixture in fixtures:
        name = fixture.stem
        blocks = 827 if name == 'm5_addy' else 345
        ref = ROOT/'build'/f'{name}-reference.wav'
        cmd = [str(SC), '-U', str(SC.parent/'plugins'), '-N', str(fixture.with_suffix('.osc')), '_', str(ref), '44100', 'WAV', 'float', '-o', '2', '-i', '0', '-z', '64']
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
        if name.startswith(('m2_', 'm3_', 'm4_', 'm5_')):
            peak_limit, rms_limit = 0.000005, 0.000001
            if name.startswith('m2_LagUD_ar'):
                peak_limit, rms_limit = 0.00002, 0.000008
        if name.startswith(('curve', 'env_')) or name in ('adsr','forced','loop','retrigger','control_env'):
            peak_limit, rms_limit = 0.000005, 0.000001
        results.append(dict(name=name, peak_error=peak, rms_error=rms, peak_limit=peak_limit, rms_limit=rms_limit, reference_rms=signal, passed=peak<peak_limit and rms<rms_limit))
    (ROOT/'build/reference-results.json').write_text(json.dumps(results, indent=2)+'\n')
    for r in results:
        print(('PASS' if r['passed'] else 'FAIL'), r['name'], r.get('error') or f"peak={r['peak_error']:.7g} rms={r['rms_error']:.7g}")
    return all(r['passed'] for r in results)
if __name__ == '__main__':
    raise SystemExit(0 if main() else 1)
