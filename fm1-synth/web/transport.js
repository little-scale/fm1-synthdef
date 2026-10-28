// Development SysEx namespace. Used only after an explicit FMS capability handshake.
export const prefix=[0x7d,0x46,0x4d,0x53,1];
export function crc32(bytes){let c=0xffffffff;for(const b of bytes){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^(0xedb88320&-(c&1));}return (c^0xffffffff)>>>0;}
export function pack7(bytes){const result=[];for(let p=0;p<bytes.length;p+=7){let high=0;const group=[];for(let i=0;i<7 && p+i<bytes.length;i++){high|=(bytes[p+i]>>>7)<<i;group.push(bytes[p+i]&127);}result.push(high,...group);}return result;}
export function unpack7(bytes){const out=[];for(let p=0;p<bytes.length;){const h=bytes[p++],count=Math.min(7,bytes.length-p);if(!count || h>=128 || h>>count)throw Error('Invalid packed SysEx');for(let i=0;i<count;i++){const b=bytes[p++];if(b>=128)throw Error('Invalid MIDI data');out.push(b|((h>>i&1)<<7));}}return Uint8Array.from(out);}
export function frame(command,id,payload=new Uint8Array()){
 if(payload.length>246)throw Error('Frame too large');const raw=new Uint8Array(payload.length+10),view=new DataView(raw.buffer);raw[0]=1;raw[1]=command;view.setUint16(2,id,true);view.setUint16(4,payload.length,true);raw.set(payload,6);view.setUint32(raw.length-4,crc32(raw.slice(0,-4)),true);return Uint8Array.from([0xf0,...prefix,...pack7(raw),0xf7]);
}
export function parse(bytes){
 if(bytes[0]!==0xf0 || bytes.at(-1)!==0xf7 || prefix.some((x,i)=>bytes[i+1]!==x))return null;
 const raw=unpack7(bytes.slice(6,-1));if(raw.length<10 || raw.length>256)throw Error('Invalid frame size');const view=new DataView(raw.buffer);
 if(raw[0]!==1 || view.getUint16(4,true)!==raw.length-10 || view.getUint32(raw.length-4,true)!==crc32(raw.slice(0,-4)))throw Error('Frame CRC/version mismatch');
 return {command:raw[1],id:view.getUint16(2,true),payload:raw.slice(6,-4)};
}
export class MidiLink{
 constructor(input,output){this.input=input;this.output=output;this.id=0;this.pending=null;this.listener=e=>{let reply;try{reply=parse(e.data);}catch{return;}const p=this.pending;if(reply && p && reply.id===p.id && reply.command===(p.command|128)){clearTimeout(p.timer);this.pending=null;reply.payload[0]===0?p.resolve(reply.payload.slice(1)):p.reject(Error(`FM-1 slot error ${reply.payload[0]}`));}};input.addEventListener('midimessage',this.listener);}
 close(){this.input.removeEventListener('midimessage',this.listener);if(this.pending){clearTimeout(this.pending.timer);this.pending.reject(Error('Disconnected'));this.pending=null;}}
 request(command,payload=new Uint8Array()){
  if(this.pending)return Promise.reject(Error('A slot request is already in progress'));
  const id=++this.id&65535,packet=frame(command,id,payload);
  return new Promise((resolve,reject)=>{const p={id,command,resolve,reject,tries:0};this.pending=p;const send=()=>{if(this.pending!==p)return;if(p.tries++===3){this.pending=null;reject(Error('No reply from slot-capable firmware. Read back the slot before retrying a save.'));return;}try{this.output.send(packet);}catch(e){this.pending=null;reject(e);return;}p.timer=setTimeout(send,1500);};send();});
 }
 async hello(){const data=await this.request(0);if(data.length!==19 || data[0]!==2 || data[1]!==9)throw Error('Incompatible slot firmware');const view=new DataView(data.buffer,data.byteOffset);return {version:2,slots:9,effectCapacity:view.getUint32(15,true),voices:data[2],capacity:view.getUint32(3,true),chunk:view.getUint32(7,true),delayBytes:view.getUint32(11,true)};}
}
