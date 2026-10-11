import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {promises as fs} from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..'),home=path.join(root,'.cache/cd-playback');await fs.mkdir(home,{recursive:true});
const child=spawn(path.join(root,'runtime/node/node.exe'),[path.join(root,'backend/entry.mjs')],{cwd:root,env:{...process.env,RHINE_APP_HOME:home,MUSIC_DATA_DIR:path.join(home,'music-data')},windowsHide:true,stdio:['pipe','pipe','pipe']});
let buffer='',counter=0,ready,state,library,errors='',pending=new Map();const report={drive:'F:',rawCdRipping:false,sonyProcessedAudioCached:true,isolatedPreferences:true};
child.stderr.on('data',chunk=>errors+=chunk);child.stdout.on('data',chunk=>{buffer+=chunk;let end;while((end=buffer.indexOf('\n'))>=0){const value=JSON.parse(buffer.slice(0,end));buffer=buffer.slice(end+1);if(value.event==='ready')ready=value;if(value.event==='library')library=value.library;if(value.event==='player')state=value.state;if(value.id){const request=pending.get(value.id);if(request){clearTimeout(request.timer);pending.delete(value.id);value.ok?request.resolve(value.result):request.reject(new Error(value.error));}}}});
async function until(predicate,seconds=30){const limit=Date.now()+seconds*1000;while(!predicate()){if(state?.transport==='error')throw new Error(state.error);if(Date.now()>limit||child.exitCode!==null)throw new Error('CD wait failed: '+errors+'; '+JSON.stringify(state));await new Promise(resolve=>setTimeout(resolve,60));}}
const call=(op,args={})=>new Promise((resolve,reject)=>{const id=++counter,timer=setTimeout(()=>{pending.delete(id);reject(new Error('RPC timeout: '+op));},60000);pending.set(id,{resolve,reject,timer});child.stdin.write(JSON.stringify({id,op,args})+'\n');});
try{
 await until(()=>ready&&library?.albums.some(a=>a.cd?.drive==='F:'));
 await call('preferences',{songFade:true,volume:.01,sony:{dsee:false,dc:0,vinyl:0},output:{backend:'wasapi',device:-1,channel:0,dsdMode:'pcm'}});
 const cd=library.albums.find(a=>a.cd?.drive==='F:'),track=cd.tracks.find(t=>t.duration>60);assert(track);
 await call('play',{albumId:cd.id,trackId:track.id});await until(()=>state?.playing&&state.currentTime>.4);assert(Math.abs(state.duration-track.duration)<.1);report.digitalPlayback={duration:state.duration,positionAdvanced:true,backend:state.backend};
 await call('toggle');await until(()=>state.transport==='paused');const paused=state.currentTime;await new Promise(resolve=>setTimeout(resolve,600));assert(Math.abs(state.currentTime-paused)<.12);report.pause=true;
 await call('seek',{position:20});await until(()=>Math.abs(state.currentTime-20)<.2);await call('toggle');await until(()=>state.playing&&state.currentTime>20.3);report.seekResume=true;
 await call('preferences',{sony:{dsee:false,dc:2,vinyl:0}});await until(()=>state.sonyProcessing?.state==='active'&&state.playing,60);assert(state.currentTime>19);report.sony={backend:state.sonyProcessing.source||state.sonyProcessing.state,positionPreserved:true,streaming:state.sonyProcessing.streaming===true};
 await call('next');await until(()=>state.currentTrack?.id!==track.id&&state.playing);report.next=true;
 await call('stop');await until(()=>state.transport==='idle');report.stop=true;report.ok=true;
}catch(error){report.ok=false;report.error=error.message;throw error;}
finally{await call('stop').catch(()=>{});await call('shutdown').catch(()=>{});child.stdin.end();for(let i=0;i<100&&child.exitCode===null;i++)await new Promise(resolve=>setTimeout(resolve,50));if(child.exitCode===null)child.kill();if(child.exitCode!==null){const cache=path.resolve(home,'music-data/sony-pcm-cache');assert.equal(path.dirname(cache),path.resolve(home,'music-data'));assert(cache.startsWith(home+path.sep));await fs.rm(cache,{recursive:true,force:true});report.sonyProcessedAudioCacheRemoved=true;}await fs.writeFile(path.join(root,'build/cd-playback.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));}
