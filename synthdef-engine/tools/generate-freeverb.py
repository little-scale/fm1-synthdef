#!/usr/bin/env python3
"""Preserve pinned SC recurrence order while moving state to the caller arena."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2];src=(root/'synthdef-engine/reference/ReverbUGens.cpp').read_text()
text='''/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SuperCollider 3.14.1 ReverbUGens.cpp.
 * Copyright (c) 2005 blackrain <blackrain.sc@gmail.com>, GPL-2.0-or-later.
 * Generated with tools/generate-freeverb.py; arithmetic/order retained.
 * All rings, counters and recurrence state live in the caller's float arena.
 */
'''
for name in ('FreeVerb','FreeVerb2'):
 struct=src.split('struct '+name+' : public Unit {')[1].split('};')[0]
 fields=re.findall(r'(int|float) (\w+)(?:\[(\d+)\])?;',struct)
 offsets={};offset=0
 for kind,key,sz in fields:offsets[key]=offset;offset+=int(sz or 1)
 print(name,offset,'float words',offset*4,'bytes')
 start=src.index('void '+name+'_next('+name+'* unit, int inNumSamples) {');end=src.index('\n}',start)+2
 body=src[start:end];body=body[body.index('{')+1:-1]
 body=re.sub(r'IN0\((\d+)\)',lambda m:'input(g,b,n,'+m[1]+',0)',body)
 body=re.sub(r'IN\((\d+)\)',lambda m:'b->signal[g->node[g->ref[n->input+'+m[1]+'].node].output+g->ref[n->input+'+m[1]+'].output]',body)
 body=re.sub(r'OUT\((\d+)\)',lambda m:'b->signal[n->output+'+m[1]+']',body)
 for kind,key,sz in fields:
  body=body.replace('unit->'+key+(';' if sz else ' '),('mem+'+str(offsets[key])+';' if sz else 'mem['+str(offsets[key])+'] '))
 # Remaining scalar reads (and no prefix collisions).
 body=re.sub(r'unit->(\w+)',lambda m:('mem+'+str(offsets[m[1]])) if next(x[2] for x in fields if x[1]==m[1]) else 'mem['+str(offsets[m[1]])+']',body)
 body=body.replace('inNumSamples','count')
 # Explicit float literals avoid accidental soft-double code on the MCU.
 body=re.sub(r'(?<![\w.])(1|0)\.(?![\d\w])',r'\1.0f',body)
 text+='static __attribute__((noinline)) void render_'+name.lower()+'(const fm_graph *g,fm_scratch *b,const fm_node *n,float *mem,int count) {'+body+'\n}\n'
(root/'synthdef-engine/src/fm_freeverb.inc').write_text(text)
