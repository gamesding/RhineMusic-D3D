import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {promises as fs} from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import http from 'node:http';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),home=path.join(root,'.cache/backend-smoke');await fs.mkdir(home,{recursive:true});
const child=spawn(path.join(root,'runtime/node/node.exe'),[path.join(root,'backend/entry.mjs')],{cwd:root,env:{...process.env,RHINE_APP_HOME:home,MUSIC_DATA_DIR:path.join(home,'music-data')},windowsHide:true,stdio:['pipe','pipe','pipe']});
let buffer='',counter=0,ready,library,errors='',pending=new Map(),waiters=[];
child.stderr.on('data',chunk=>errors+=chunk);
child.stdout.on('data',chunk=>{buffer+=chunk;let end;while((end=buffer.indexOf('\n'))>=0){const value=JSON.parse(buffer.slice(0,end));buffer=buffer.slice(end+1);if(value.event==='ready')ready=value;if(value.event==='library')library=value.library;if(value.id){const request=pending.get(value.id);if(request){clearTimeout(request.timer);pending.delete(value.id);value.ok?request.resolve(value.result):request.reject(new Error(value.error));}}for(const waiter of waiters)waiter();}});
async function until(predicate){const limit=Date.now()+30000;while(!predicate()){if(Date.now()>limit||child.exitCode!==null)throw new Error('Service wait failed: '+errors);await new Promise(resolve=>setTimeout(resolve,50));}}
const call=(op,args={})=>new Promise((resolve,reject)=>{const id=++counter,timer=setTimeout(()=>{pending.delete(id);reject(new Error('RPC timeout: '+op));},60000);pending.set(id,{resolve,reject,timer});child.stdin.write(JSON.stringify({id,op,args})+'\n');});
const report={};
try{
 await until(()=>ready&&library);assert.equal(ready.backend,'native-d3d12');report.transportCore=true;
 const foreign=await fetch(ready.origin+'/api/library',{headers:{Origin:'http://evil.invalid'}});assert.equal(foreign.status,403);
 const hostStatus=await new Promise((resolve,reject)=>{const request=http.get(ready.origin+'/api/library',{headers:{Host:'evil.invalid'}},response=>{response.resume();resolve(response.statusCode);});request.once('error',reject);});assert.equal(hostStatus,403);report.loopbackProtection=true;
 const audio=await call('request',{path:'/api/native/status'});assert.equal(audio.available,true);report.native=audio;
 const sony=await call('request',{path:'/api/sony/status'});assert.equal(sony.available,true);assert.equal(sony.backend,'x86-cpp');report.sony=sony;
 await call('preferences',{theme:'night',songFade:false,volume:.02});let state=await call('state');assert.equal(state.preferences.theme,'night');assert.equal(state.player.songFadeEnabled,false);assert.equal(state.player.volume,.02);
 const saved=JSON.parse(await fs.readFile(path.join(home,'preferences.json'),'utf8'));assert.equal(saved.theme,'night');report.preferences=true;
 const cd=await call('cd:refresh');assert(cd.albums.some(a=>a.cd.drive==='F:'));report.cdTracks=cd.albums.find(a=>a.cd.drive==='F:').tracks.length;
 await call('shutdown');child.stdin.end();await until(()=>child.exitCode!==null);assert.equal(child.exitCode,0);report.cleanShutdown=true;report.ok=true;
}finally{if(child.exitCode===null){await call('shutdown').catch(()=>{});child.stdin.end();await until(()=>child.exitCode!==null).catch(()=>child.kill());}await fs.writeFile(path.join(root,'build/backend-smoke.json'),JSON.stringify(report,null,2)+'\n');}
console.log(JSON.stringify(report,null,2));
