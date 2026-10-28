// Display model shared by the editor and the 240 × 240 preview.
export const colors={text:'#000000',keyword:'#bf0000',class:'#0000bf',number:'#800080',comment:'#737373',string:'#008000',symbol:'#008000'};
export function tokens(source){
 const out=[];let i=0;
 while(i<source.length){
  const start=i;let kind='text',match;
  if(source.startsWith('//',i)){kind='comment';while(i<source.length && source[i]!='\n')i++;}
  else if(source.startsWith('/*',i)){kind='comment';i+=2;let depth=1;while(i<source.length && depth){if(source.startsWith('/*',i)){depth++;i+=2;}else if(source.startsWith('*/',i)){depth--;i+=2;}else i++;}}
  else if(source[i]=='"' || source[i]=="'"){kind=source[i]=='"'?'string':'symbol';const quote=source[i++];while(i<source.length){const c=source[i++];if(c=='\\' && i<source.length)i++;else if(c==quote)break;}}
  else if((match=/^\\[a-zA-Z_]\w*/.exec(source.slice(i)))){kind='symbol';i+=match[0].length;}
  else if((match=/^(?:0x[0-9a-fA-F]+|\d+r[0-9a-zA-Z]+|(?:\d+\.\d+|\.\d+|\d+)(?:[eE][+-]?\d+)?)/.exec(source.slice(i)))){kind='number';i+=match[0].length;}
  else if((match=/^[a-zA-Z_]\w*/.exec(source.slice(i)))){const name=match[0];kind=['arg','var'].includes(name)?'keyword':/^[A-Z]/.test(name)?'class':'text';i+=name.length;}
  else i++;
  out.push({text:source.slice(start,i),kind});
 }
 return out;
}
export function wrap(source,columns=36){
 const lines=[[]];let col=0;
 for(const token of tokens(source))for(const original of token.text){
  if(original=='\r')continue;
  if(original=='\n'){lines.push([]);col=0;continue;}
  const chars=original=='\t'?' '.repeat(4-col%4):original;
  for(const char of chars){if(col==columns){lines.push([]);col=0;}lines.at(-1).push({char,kind:token.kind});col++;}
 }
 return lines;
}
export function drawScreen(canvas,{source='',slot=0,name='Empty slot',scroll=0,knobs=[0.5,0.5,0.5,0.5],overlay=-1,levels=[0,0],octave=0,draft=false}={}){
 const c=canvas.getContext('2d');c.fillStyle='white';c.fillRect(0,0,240,240);c.fillStyle='black';c.font='bold 11px Monaco, Menlo, monospace';
 c.fillText(`${String(slot===8?0:slot+1).padStart(2,'0')}${slot===8?' FX':' / 08'}   ${draft?'DRAFT':name}`.slice(0,32),7,15);
 c.strokeStyle='#ddd';c.beginPath();c.moveTo(6,23);c.lineTo(234,23);c.stroke();
 const lines=wrap(source || '// Source unavailable\n// Import a .scd companion or\n// paste and compile code.');const top=Math.max(0,Math.min(scroll,Math.max(0,lines.length-15)));
 c.font='10px Monaco, Menlo, monospace';
 for(let row=0;row<15;row++)for(const [col,ch] of (lines[top+row]||[]).entries()){c.fillStyle=colors[ch.kind];c.fillText(ch.char,7+col*6.2,36+row*12);}
 c.fillStyle='#555';c.font='9px Monaco, Menlo, monospace';c.fillText(`${top+1}–${Math.min(top+15,lines.length)}/${lines.length}  OCT ${octave>=0?'+':''}${octave}`,7,224);
 for(let ch=0;ch<2;ch++){const x=128,y=215+ch*11;c.fillStyle='#000';c.fillText(ch?'R':'L',116,y+6);c.fillStyle='#eee';c.fillRect(x,y,97,5);c.fillStyle=levels[ch]>=1?'#bf0000':'#111';c.fillRect(x,y,97*Math.max(0,Math.min(1,(20*Math.log10(Math.max(0.001,levels[ch]))+60)/60)),5);}
 if(overlay>=0){c.fillStyle='white';c.fillRect(16,165,208,43);c.strokeStyle='#111';c.strokeRect(16,165,208,43);c.fillStyle='#111';c.font='12px Monaco, Menlo, monospace';c.fillText(`knob${overlay+1}  ${knobs[overlay].toFixed(2)}`,25,184);c.fillRect(25,193,190*knobs[overlay],4);}
 return top;
}
