#!/usr/bin/env python3
"""Build the pinned upstream MiVerb oracle; never load arbitrary user plugins."""
import hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def build():
    for folder in ('mi-ugens','sc-headers'):
        base=ROOT/'reference'/folder
        for path,digest in json.loads((base/'sources.json').read_text())['sha256'].items():
            if hashlib.sha256((base/path).read_bytes()).hexdigest()!=digest:
                raise RuntimeError('Modified reference source: '+str(base/path))
    out=ROOT/'build/reverb-plugins';out.mkdir(parents=True,exist_ok=True)
    mi=ROOT/'reference/mi-ugens';sc=ROOT/'reference/sc-headers/include'
    subprocess.run(['c++','-std=c++17','-DTEST','-O2','-fno-fast-math','-ffp-contract=off','-fPIC','-bundle','-undefined','dynamic_lookup',
        '-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),'-I'+str(mi/'projects/MiVerb/include'),'-I'+str(mi/'eurorack'),
        str(mi/'projects/MiVerb/MiVerb.cpp'),'-o',str(out/'MiVerb.scx')],check=True)
    subprocess.run(['c++','-std=c++17','-DTEST','-O2','-fno-fast-math','-ffp-contract=off','-fPIC','-bundle','-undefined','dynamic_lookup',
        '-I'+str(sc/'plugin_interface'),'-I'+str(sc/'common'),'-I'+str(mi/'eurorack'),
        str(mi/'projects/MiMu/MiMu.cpp'),'-o',str(out/'MiMu.scx')],check=True)
    return out
if __name__=='__main__':print(build())
