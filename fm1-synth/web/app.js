import {tokens,colors,drawScreen,wrap} from './screen.js';
const $=id=>document.getElementById(id);
const token=new URLSearchParams(location.hash.slice(1)).get('token')||sessionStorage.getItem('fm1-token');
if(token)sessionStorage.setItem('fm1-token',token);history.replaceState(null,'',location.pathname);
let selected=0,revision=0,binary='',compiledSource='',unsaved=false,busy=false,info=null,used=0,scroll=0,octave=0,overlay=-1,overlayTimer,knobs=Array(8).fill(.5),meters=[],meterInterval=.03,audioURL=null;
let deviceSlots=null,lastSynth=0,lastFX=8,initializedBanks=new Set(),bankUsed=0;
const effectMode=()=>selected>=8;
const controlIndex=i=>i+(effectMode()?4:0);
const slotNumber=i=>i>=8?`FX${i-7}`:i+1;
const slotCapacity=()=>selected>=8?7916:16128;
const example=String.raw`SynthDef(\keys, {
    arg freq=440, amp=0.5, gate=1,
        knob1=0.5, knob2=0.3,
        knob3=0.2, knob4=0.5;
    var cutoff, env, sound;
    cutoff = 200 + (Lag.kr(knob1, 0.02) * 6000);
    env = EnvGen.ar(
        Env.adsr(0.01, 0.1, 0.7, 0.3),
        gate, doneAction: 2
    );
    sound = Saw.ar(freq);
    sound = RLPF.ar(sound, cutoff,
        0.1 + (knob2 * 0.8));
    sound = sound + (CombC.ar(sound,
        0.1, 0.07, 0.4) * knob3);
    Out.ar(0, Pan2.ar(
        sound * env * amp * 0.12,
        (knob4 * 2) - 1
    ));
})`;
async function api(path,data){const response=await fetch('/api/'+path,{method:data?'POST':'GET',headers:{Authorization:'Bearer '+token,...(data?{'Content-Type':'application/json'}:{})},...(data?{body:JSON.stringify(data)}:{})});const result=await response.json();if(!response.ok)throw Error(result.error||'Request failed');return result;}
function message(text,error=false){$('message').textContent=text;$('message').classList.toggle('error',error);}
function ready(){return binary && $('source').value===compiledSource;}
function paint(){
 $('highlight').replaceChildren(...tokens($('source').value).map(t=>{const span=document.createElement('span');span.style.color=colors[t.kind];span.textContent=t.text;return span;}));
 $('highlight').append(document.createTextNode('\n'));
 $('highlight').scrollTop=$('source').scrollTop;$('highlight').scrollLeft=$('source').scrollLeft;
 $('capacity').textContent=effectMode()?`${used.toLocaleString()} B · FX bank ${bankUsed.toLocaleString()} / 7,916 B`:used?`${used.toLocaleString()} / ${slotCapacity().toLocaleString()} B`: `${slotCapacity().toLocaleString()} B maximum${effectMode()?' · 7,916 B shared FX bank':''}`;
 $('state').textContent=ready()?(unsaved?'Ready to save':'Saved'):(binary?'Source changed · compile to update':'Draft · not compiled');
 $('save').disabled=busy || !ready();$('listen').disabled=busy || !ready();$('export').disabled=busy || unsaved || !binary;
 $('device-send').disabled=busy || !ready() || !deviceSlots?.[selected];$('device-read').disabled=busy || !deviceSlots?.[selected];$('device-clear').disabled=busy || !deviceSlots?.[selected];$('device-send').textContent=`Send to FM-1 slot ${slotNumber(selected)}`;
 $('info').textContent=info?`${info.nodes} graph nodes · ${(info.delay_bytes/1024).toFixed(0)} KiB delay/reverb RAM ${selected>=8?'· permanent stereo effect':'/ voice · 2 voices'}`:'';
 screen();
}
function screen(levels=[0,0]){scroll=drawScreen($('screen'),{source:$('source').value,slot:selected,name:$('name').value||'Empty slot',scroll,knobs,overlay,levels,octave,draft:unsaved});}
async function task(fn){if(busy)return;busy=true;document.querySelectorAll('button,textarea,input').forEach(el=>el.disabled=true);$('stop').disabled=false;try{await fn();}catch(e){message(e.message,true);}finally{busy=false;document.querySelectorAll('button,textarea,input').forEach(el=>el.disabled=false);paint();}}
function stop(){const audio=$('audio');audio.pause();audio.currentTime=0;screen();}
async function slots(){const state=await api('state');$('slots').replaceChildren(...state.slots.map(slot=>{const button=document.createElement('button');button.className='slot'+(slot.slot===selected?' active':'')+(slot.empty?' empty':'');const number=document.createElement('b');number.textContent=String(slotNumber(slot.slot)).padStart(2,'0');const title=document.createElement('span');title.textContent=slot.empty?'Empty':slot.name;button.append(number,title);button.onclick=()=>select(slot.slot);return button;}));}
function accept(result,resetControls=true){bankUsed=result.bank_used??bankUsed;revision=result.revision??revision;binary=result.binary||'';info=result.info||null;used=result.used||0;compiledSource=result.source||'';$('source').value=compiledSource;$('name').value=result.name||'';if(resetControls)setDefaults(result);updateKnobs();scroll=0;}
function setDefaults(result){const defaults=result.knob_defaults||Array.from({length:8},(_,i)=>Math.max(0,Math.min(1,info?.controls?.['knob'+(i+1)]??.5)));if(initializedBanks.size===0)knobs=defaults.slice();const start=effectMode()?4:0;for(let k=start;k<start+4;k++)knobs[k]=defaults[k];initializedBanks.add(effectMode());}
async function select(index){if(busy)return;if(unsaved && !confirm('Discard the current unsaved draft?'))return;await task(async()=>{stop();selected=(index+12)%12;const reset=!initializedBanks.has(effectMode())||selected!==(effectMode()?lastFX:lastSynth);if(selected>=8)lastFX=selected;else lastSynth=selected;accept(await api('slot/'+selected),reset);unsaved=false;await slots();message(selected>=8?'ALGORITHM selects FX1–FX4 or bypass. Press FX to toggle K5–K8 and effect code. Four effects share 7,916 bytes.':binary?'Slot loaded from the local bank.':'Empty slot. Paste code or import a file.');});}
function updateKnobs(){for(let i=0;i<4;i++){const k=controlIndex(i);$('knob'+i).value=knobs[k];$('value'+i).textContent=knobs[k].toFixed(2);$('label'+i).textContent='KNOB '+(k+1);$('knob'+i).setAttribute('aria-label','Knob '+(k+1));}$('fx-toggle').setAttribute('aria-pressed',String(effectMode()));$('fx-toggle').textContent=effectMode()?'FX ON · K5–K8':'FX OFF · K1–K4';}
for(let i=0;i<4;i++){const label=document.createElement('label');label.innerHTML=`<span><span id="label${i}">KNOB ${i+1}</span> <output id="value${i}">0.50</output></span><input id="knob${i}" aria-label="Knob ${i+1}" type="range" min="0" max="1" step="any" value="0.5">`;$('knobs').append(label);$('knob'+i).onkeydown=e=>{if(['ArrowLeft','ArrowDown','ArrowRight','ArrowUp'].includes(e.key)){e.preventDefault();e.target.value=Math.max(0,Math.min(1,knobs[controlIndex(i)]+(['ArrowLeft','ArrowDown'].includes(e.key)?-.02:.02)));e.target.dispatchEvent(new Event('input'));}};$('knob'+i).oninput=()=>{const k=controlIndex(i);knobs[k]=Number($('knob'+i).value);$('value'+i).textContent=knobs[k].toFixed(2);overlay=k;clearTimeout(overlayTimer);overlayTimer=setTimeout(()=>{overlay=-1;screen();},1200);screen();};}
$('source').oninput=()=>{unsaved=true;paint();};$('name').oninput=()=>{unsaved=true;paint();};$('source').onscroll=()=>{$('highlight').scrollTop=$('source').scrollTop;$('highlight').scrollLeft=$('source').scrollLeft;};
$('source').onkeydown=e=>{if(e.key==='Tab'){e.preventDefault();const field=e.target;field.setRangeText('    ',field.selectionStart,field.selectionEnd,'end');field.dispatchEvent(new Event('input'));}};
$('compile').onclick=()=>task(async()=>{message('Compiling with SuperCollider…');const source=$('source').value;const result=await api('compile',{slot:selected,source,name:$('name').value});binary=result.binary;compiledSource=source;info=result.info;used=result.used;if(!$('name').value)$('name').value=info.name;unsaved=true;setDefaults(result);updateKnobs();message('Compiled and validated. Listen, then save when ready.');});
$('save').onclick=()=>task(async()=>{accept(await api('save',{slot:selected,revision,name:$('name').value,source:compiledSource,binary}));unsaved=false;await slots();message('Saved to the local bank with readback verification.');});
$('clear').onclick=()=>{if(confirm(`Clear local slot ${slotNumber(selected)}? Export it first if you want a copy.`))task(async()=>{accept(await api('clear',{slot:selected,revision}));unsaved=false;await slots();stop();message('Slot cleared.');});};
$('import').onclick=()=>$('file').click();
function toBase64(buffer){return btoa(Array.from(new Uint8Array(buffer),x=>String.fromCharCode(x)).join(''));}
function bytes(value){return Uint8Array.from(atob(value),x=>x.charCodeAt(0));}
$('file').onchange=()=>task(async()=>{const file=$('file').files[0];$('file').value='';if(!file)return;if(file.size>65536)throw Error('File is too large.');if(unsaved && !confirm('Replace the current unsaved draft?'))return;stop();if(file.name.endsWith('.scd')){$('source').value=await file.text();binary='';info=null;used=0;message('Source imported. Compile before saving.');}else if(file.name.endsWith('.fmslot')){const result=await api('package',{slot:selected,package:toBase64(await file.arrayBuffer())});accept(result);message('Slot package loaded as a draft. Save to replace the selected slot.');}else if(file.name.endsWith('.scsyndef')){const encoded=toBase64(await file.arrayBuffer()),result=await api('validate',{slot:selected,binary:encoded});accept({...result,binary:encoded,name:result.info.name,source:''});message('Compiled graph imported. Original source is unavailable; no code has been invented.');}else throw Error('Choose .scd, .scsyndef or .fmslot.');unsaved=true;});
$('export').onclick=()=>task(async()=>{const result=await api('slot/'+selected);const url=URL.createObjectURL(new Blob([bytes(result.package)],{type:'application/octet-stream'}));const a=document.createElement('a');a.href=url;a.download=`slot-${slotNumber(selected)}.fmslot`;a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);message('Exported source, compiled graph and metadata in one slot package.');});
$('listen').onclick=()=>task(async()=>{stop();message('Rendering note…');const midi=Number($('midi').value)+12*octave;const result=await api('preview',{slot:selected,binary,midi,velocity:Number($('velocity').value),knobs,preview_slot:lastSynth,effect_slot:$('bypass').checked?-1:lastFX});if(audioURL)URL.revokeObjectURL(audioURL);audioURL=URL.createObjectURL(new Blob([bytes(result.wav)],{type:'audio/wav'}));meters=result.meters;meterInterval=result.meter_interval;$('audio').src=audioURL;await $('audio').play();message('Host preview: one second held, then release and effect tail.');});
$('stop').onclick=stop;$('up').onclick=()=>{scroll=Math.max(0,scroll-1);screen();};$('down').onclick=()=>{scroll=Math.min(Math.max(0,wrap($('source').value).length-15),scroll+1);screen();};$('previous').onclick=()=>select((lastSynth+7)%8);$('next').onclick=()=>select((lastSynth+1)%8);
$('algorithm-prev').onclick=()=>select(8+(lastFX-8+3)%4);$('algorithm-next').onclick=()=>select(8+(lastFX-8+1)%4);$('fx-toggle').onclick=()=>select(effectMode()?lastSynth:lastFX);
for(const [id,delta] of [['octup',1],['octdown',-1]])$(id).onclick=()=>{octave=Math.max(-2,Math.min(2,octave+delta));$('octave').textContent=(octave>=0?'+':'')+octave;screen();};
function animate(){const a=$('audio');if(!a.paused)screen(meters[Math.min(meters.length-1,Math.floor(a.currentTime/meterInterval))]||[0,0]);requestAnimationFrame(animate);}requestAnimationFrame(animate);$('audio').onended=()=>screen();$('audio').onpause=()=>screen();
window.addEventListener('beforeunload',e=>{if(unsaved){e.preventDefault();e.returnValue='';}});
await select(0);if(!binary && !$('source').value){$('source').value=example;$('name').value='Keys + comb';unsaved=true;message('Example draft loaded. Compile and save to create your first slot.');paint();}

$('device-check').onclick=()=>task(async()=>{message('Checking FM-1 and reading slot names…');const r=await api('device/info',{});deviceSlots=r.slots;document.querySelector('.status').textContent=`FM-1 connected · ${r.voices} voices · master FX · local editor`;message('FM-1 slots: '+r.slots.map(s=>`${slotNumber(s.slot)}: ${s.name}`).join(' · '));});
$('device-send').onclick=()=>task(async()=>{message('Sending to FM-1; notes pause while the slot is saved…');const r=await api('device/write',{slot:selected,revision:deviceSlots[selected].revision,name:$('name').value,source:compiledSource,binary});if(selected>=8)for(const item of deviceSlots.slice(8))item.revision=r.revision;else deviceSlots[selected].revision=r.revision;deviceSlots[selected].name=$('name').value;message(`FM-1 slot ${slotNumber(selected)} saved and read back successfully. ${selected>=8?'Turn ALGORITHM to select this effect.':'Turn PRESET on the FM-1 to select it.'}`);});
$('device-read').onclick=()=>{if(unsaved && !confirm('Replace the current draft with the FM-1 slot?'))return;task(async()=>{const r=await api('device/read',{slot:selected});deviceSlots[selected].revision=r.device_revision;accept(r);unsaved=true;message(`Read FM-1 slot ${slotNumber(selected)} into the editor. Save slot keeps a copy on this Mac.`);});};

$('device-clear').onclick=()=>{
 if(!confirm(selected>=8?'Clear this FX preset on the FM-1?':`Clear synth slot ${slotNumber(selected)} on the FM-1?`))return;
 task(async()=>{const r=await api('device/write',{slot:selected,revision:deviceSlots[selected].revision,clear:true});if(selected>=8)for(const item of deviceSlots.slice(8))item.revision=r.revision;else deviceSlots[selected].revision=r.revision;message(selected>=8?'Effect preset cleared.':'Device slot cleared.');});
};
