#!/usr/bin/env python3
"""Loopback slot manager with explicit, capability-checked device transfers."""
import argparse
import base64
from http.server import BaseHTTPRequestHandler,HTTPServer
import json
import mimetypes
import os
import secrets
from urllib.parse import urlparse
from bank import ROOT,Bank,pack,unpack,validate,compile_source,preview,SLOT_ORDER,FX_SLOT,capacity

def serve(path,port=8765):
    bank=Bank(path);token=os.environ.get('FMS_SESSION_TOKEN') or secrets.token_urlsafe(32)
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def send(self,status,data,kind='application/json'):
            if kind=='application/json':data=json.dumps(data,allow_nan=False).encode()
            self.send_response(status);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(len(data)));self.send_header('Cache-Control','no-store');self.send_header('X-Content-Type-Options','nosniff');self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; media-src 'self' blob:; img-src 'self' data:; frame-ancestors 'none'");self.end_headers();self.wfile.write(data)
        def allowed(self):
            host=self.headers.get('Host','');origin=self.headers.get('Origin')
            return host in (f'127.0.0.1:{port}',f'localhost:{port}') and (not origin or origin==f'http://{host}')
        def auth(self):return self.allowed() and secrets.compare_digest(self.headers.get('Authorization',''),'Bearer '+token)
        def do_GET(self):
            path=urlparse(self.path).path
            if not self.allowed():self.send(403,{'error':'Local access only.'});return
            if path.startswith('/api/'):
                if not self.auth():self.send(403,{'error':'Open the manager using its launch link.'});return
                try:
                    if path=='/api/state':result=dict(mode='local',device_connected=False,slots=[bank.describe(i) for i in SLOT_ORDER],voices=2,capacity=16128,delay_bytes_per_voice=65536)
                    elif path.startswith('/api/slot/'):result=bank.describe(int(path.rsplit('/',1)[1]),True)
                    else:self.send(404,{'error':'Not found'});return
                    self.send(200,result)
                except (ValueError,KeyError,UnicodeError) as e:self.send(400,{'error':str(e)})
                return
            names={'/':'index.html','/app.js':'app.js','/screen.js':'screen.js','/style.css':'style.css'}
            if path not in names:self.send(404,{'error':'Not found'});return
            file=ROOT/'web'/names[path];self.send(200,file.read_bytes(),mimetypes.guess_type(file)[0] or 'text/plain')
        def do_POST(self):
            if not self.auth():self.send(403,{'error':'Local session required.'});return
            try:
                n=int(self.headers.get('Content-Length','0'))
                if not 0<n<=100000:raise ValueError('Request too large or empty.')
                def bad_constant(value):raise ValueError('Non-finite JSON number: '+value)
                data=json.loads(self.rfile.read(n),parse_constant=bad_constant)
                path=urlparse(self.path).path
                effect=data.get('slot')==FX_SLOT
                if path=='/api/compile':
                    binary,info=compile_source(data['source'],effect);payload=pack(data.get('name') or info['name'],data['source'],binary,effect)
                    result=dict(binary=base64.b64encode(binary).decode(),info=info,used=len(payload),knob_defaults=bank.knob_defaults(data.get('slot',0),info))
                elif path=='/api/validate':
                    binary=base64.b64decode(data['binary'],validate=True);_,info=validate(binary,effect)
                    result=dict(info=info,used=len(pack(data.get('name') or info['name'],'',binary,effect)))
                elif path=='/api/save':
                    binary=base64.b64decode(data['binary'],validate=True);payload=pack(data['name'],data.get('source',''),binary,effect)
                    bank.put(data['slot'],payload,data['revision']);result=bank.describe(data['slot'],True)
                elif path=='/api/clear':
                    bank.put(data['slot'],b'',data['revision']);result=bank.describe(data['slot'],True)
                elif path=='/api/import':
                    payload=base64.b64decode(data['package'],validate=True);unpack(payload);bank.put(data['slot'],payload,data['revision']);result=bank.describe(data['slot'],True)
                elif path=='/api/package':
                    payload=base64.b64decode(data['package'],validate=True);name,source,binary=unpack(payload);_,info=validate(binary,effect)
                    result=dict(name=name,source=source,binary=base64.b64encode(binary).decode(),info=info,used=len(payload),knob_defaults=bank.knob_defaults(data.get('slot',0),info))
                elif path in ('/api/device/info','/api/device/read','/api/device/write'):
                    from device import Device
                    dev=Device()
                    try:
                        if path.endswith('/info'):
                            slots=[]
                            for i in range(9):
                                body,rev=dev.read(i)
                                name=unpack(body)[0] if body else 'Empty'
                                slots.append(dict(slot=i,revision=rev,name=name,empty=not body))
                            result=dict(port=dev.port,voices=dev.info[2],slots=slots,stats=dev.stats())
                        else:
                            slot=data['slot']
                            if type(slot)!=int or not 0<=slot<9:raise ValueError('Slot must be 0–8.')
                            if path.endswith('/write'):
                                if data.get('clear'):body=b''
                                else:
                                    binary=base64.b64decode(data['binary'],validate=True)
                                    body=pack(data['name'],data.get('source',''),binary,effect)
                                expected=data['revision']
                                if type(expected)!=int or not 0<=expected<2**32-1:raise ValueError('Check the FM-1 slot revision first.')
                                rev=dev.write(slot,body,expected)
                                result=dict(slot=slot,revision=rev,used=len(body))
                            else:
                                body,rev=dev.read(slot)
                                if body:
                                    name,source,binary=unpack(body);_,info=validate(binary,effect)
                                    result=dict(name=name,source=source,binary=base64.b64encode(binary).decode(),info=info,used=len(body),device_revision=rev)
                                else:result=dict(empty=True,name='',source='',binary='',used=0,device_revision=rev)
                    finally:dev.close()
                elif path=='/api/preview':
                    binary=base64.b64decode(data['binary'],validate=True)
                    if effect:
                        effect_binary=binary;voice,_=bank.get(data.get('preview_slot',0))
                        binary=unpack(voice)[2] if voice else (ROOT/'examples/keys-master.scsyndef').read_bytes()
                    else:
                        fx,_=bank.get(FX_SLOT);effect_binary=unpack(fx)[2] if fx else None
                    audio,meters=preview(binary,data.get('midi',60),data.get('velocity',90),data.get('knobs'),effect_binary)
                    result=dict(wav=base64.b64encode(audio).decode(),meters=meters,meter_interval=20*64/44100)
                else:self.send(404,{'error':'Not found'});return
                self.send(200,result)
            except (ValueError,TypeError,KeyError,OSError,ImportError) as e:self.send(400,{'error':str(e)})
    server=HTTPServer(('127.0.0.1',port),Handler)
    print(f'FM-1 local bank: http://127.0.0.1:{port}/#token={token}',flush=True)
    print('Local bank by default. Explicit Send to FM-1 writes the selected device slot after a capability check.',flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bank',default=str(ROOT/'data/local-bank.flash'));p.add_argument('--port',type=int,default=8765);a=p.parse_args();serve(a.bank,a.port)
