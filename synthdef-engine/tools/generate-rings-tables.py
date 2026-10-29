#!/usr/bin/env python3
"""Extract exact tables from the pinned MIT Rings/stmlib reference sources."""
from pathlib import Path
import json,re,hashlib
root=Path(__file__).resolve().parents[1];up=root/'reference/mi-rings';manifest=json.loads((up/'sources.json').read_text())
for rel,digest in manifest['sha256'].items():assert hashlib.sha256((up/rel).read_bytes()).hexdigest()==digest,rel
tables=[]
for rel,names in [('eurorack/rings/resources.cc',['lut_sine','lut_4_decades','lut_svf_shift','lut_stiffness','lut_fm_frequency_quantizer']),('eurorack/stmlib/dsp/units.cc',['lut_pitch_ratio_high','lut_pitch_ratio_low'])]:
 for name in names:
  m=re.search(r'const float '+name+r'\[[^\]]*\]\s*=\s*\{.*?\};',(up/rel).read_text(),re.S);assert m
  tables.append(m.group().replace('const float '+name,'static const float rg_'+name))
s=(up/'eurorack/rings/dsp/part.cc').read_text().split('#ifdef BRYAN_CHORDS')[1].split('#else')[0];rows=re.findall(r'\{([^{}]+)\}',s);assert len(rows)>=11
assert all(len(row.split(','))==8 for row in rows[:11])
tables.append('static const float rg_chords[11][8]={\n'+',\n'.join('{'+row.strip()+'}' for row in rows[:11])+'\n};')
(root/'src/fm_rings_tables.inc').write_text('/* Generated from pinned MIT-licensed Rings/stmlib tables.\n * Copyright 2014-2015 Emilie Gillet. Full notice: reference/mi-rings/LICENSE. */\n'+'\n'.join(tables)+'\n')
