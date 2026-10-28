#!/usr/bin/env python3
"""Regenerate pinned SuperCollider fixtures, then update their integrity manifest."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
SCLANG=os.environ.get('SCLANG','/Applications/SuperCollider.app/Contents/MacOS/sclang')
version=subprocess.check_output([SCLANG,'-v'],text=True,timeout=10)
if '3.14.1' not in version:raise SystemExit('Fixture export requires SuperCollider 3.14.1')
for script in ('export-fixtures.scd','export-stage2.scd','export-stage3.scd','export-delays.scd','export-random.scd'):
    run=subprocess.run([SCLANG,'-D',str(ROOT/'tools'/script)],capture_output=True,text=True,timeout=45)
    if run.returncode or 'ERROR:' in run.stdout+run.stderr or 'fixtures exported' not in run.stdout:
        raise SystemExit(run.stdout+run.stderr)
    print(script+': exported')
files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((ROOT/'fixtures').iterdir()) if p.suffix in ('.scsyndef','.osc','.json')}
(ROOT/'reference/fixtures.json').write_text(json.dumps({'supercollider':'3.14.1','block_size':64,'sample_rate':44100,'sha256':files},indent=2)+'\n')
