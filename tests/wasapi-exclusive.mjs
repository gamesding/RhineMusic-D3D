import assert from 'node:assert/strict';
import {promises as fs} from 'node:fs';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import {randomUUID,createHash} from 'node:crypto';
import {silentWav,dsdFixture} from '../upstream/scripts/audio-fixtures.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const directory=path.join(root,'.cache/wasapi-exclusive',randomUUID());await fs.mkdir(directory,{recursive:true});
const runtime=process.env.RHINE_TEST_RUNTIME_JSON?JSON.parse(process.env.RHINE_TEST_RUNTIME_JSON):path.join(root,'runtime');
const {NativeOutput}=await import(pathToFileURL(path.join(runtime,'scripts/native-output.mjs')));
const executable=path.join(runtime,'vendor/audio/windows-x64/rhine-audio-host.exe');
const files=new Map();for(const rate of [44100,48000,96000,192000,384000,12345]){const filename=path.join(directory,'silent-'+rate+'.wav');await fs.writeFile(filename,silentWav(3,rate,24));files.set('pcm-'+rate,filename);}
const dsd=path.join(directory,'silent-dsd64.dsf'),fixture=dsdFixture(2822400,Math.ceil(2822400*3/8));fixture.dsf.fill(0x69,92);await fs.writeFile(dsd,fixture.dsf);files.set('dsd',dsd);
const options={executable,store:{trackFile:id=>({path:files.get(id),allowedRoot:directory})}};
const host=new NativeOutput(options),other=new NativeOutput(options),report={checks:[],syntheticSourcesOnly:true,volume:0,packagedRuntime:!!process.env.RHINE_TEST_RUNTIME_JSON,nativeHostSha256:createHash('sha256').update(await fs.readFile(executable)).digest('hex')};
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const start=(output,trackId,settings)=>output.start({session:randomUUID(),trackId,settings,volume:0,position:0});
try{
 const status=await host.status(),device=status.wasapiDevices.find(d=>d.enabled&&/FiiO/i.test(d.name));assert(device,'The connected FiiO WASAPI endpoint was not enumerated');report.device=device;report.wasapiVersion=status.wasapiVersion;
 const settings={backend:'wasapi-exclusive',device:-1,channel:0,dsdMode:'pcm',wasapiDevice:device.id};
 for(const rate of [44100,48000,96000,192000,384000]){
  const opened=await start(host,'pcm-'+rate,settings),session=opened.session;assert.equal(opened.exclusive,true);assert(opened.wasapiInitFlags&1);assert.equal(opened.sampleRate,rate);assert.equal(opened.paused,true);
  await host.control({session,action:'play'});await sleep(250);const playing=await host.control({session,action:'status'});assert.equal(playing.playing,true);assert(playing.position>.05);assert.equal(playing.error,'');
  const paused=await host.control({session,action:'pause'});await sleep(100);const still=await host.control({session,action:'status'});assert(Math.abs(still.position-paused.position)<.03);assert.equal(still.paused,true);
  await host.control({session,action:'seek',position:1.2});assert(Math.abs((await host.control({session,action:'status'})).position-1.2)<.03);
  await host.control({session,action:'volume',volume:0});await host.control({session,action:'play'});await sleep(100);assert.equal((await host.control({session,action:'status'})).playing,true);await host.stopCurrent();
  report.checks.push({name:'PCM '+rate,exclusive:true,exactRate:true,playPauseSeekStop:true});
 }
 const owned=await start(host,'pcm-44100',settings);await host.control({session:owned.session,action:'play'});
 await assert.rejects(start(other,'pcm-44100',settings),/WASAPI/);await other.stopCurrent();await host.stopCurrent();
 const released=await start(other,'pcm-44100',settings);assert.equal(released.exclusive,true);await other.stopCurrent();report.checks.push({name:'two processes contend for one endpoint',busyRejected:true,releaseAndRecovery:true});
 await assert.rejects(start(host,'pcm-12345',settings),/WASAPI/);await host.stopCurrent();report.checks.push({name:'unsupported rate is rejected',noSharedFallback:true});
 await assert.rejects(start(host,'pcm-44100',{...settings,wasapiDevice:255}),/WASAPI/);await host.stopCurrent();report.checks.push({name:'unavailable endpoint is rejected',noSharedFallback:true});
 const converted=await start(host,'dsd',settings);assert.equal(converted.exclusive,true);assert.equal(converted.mode,'pcm');await host.control({session:converted.session,action:'play'});await sleep(150);assert.equal((await host.control({session:converted.session,action:'status'})).playing,true);await host.stopCurrent();report.checks.push({name:'DSD64 through exclusive PCM',sampleRate:converted.sampleRate});
 const ending=await start(host,'pcm-44100',settings);await host.control({session:ending.session,action:'seek',position:2.8});await host.control({session:ending.session,action:'play'});
 const limit=Date.now()+5000;let ended;do{await sleep(60);ended=await host.control({session:ending.session,action:'status'});}while(!ended.ended&&Date.now()<limit);assert.equal(ended.ended,true);assert.equal(ended.exclusive,false);
 const eofReleased=await start(other,'pcm-44100',settings);assert.equal(eofReleased.exclusive,true);await other.stopCurrent();await host.stopCurrent();report.checks.push({name:'EOF releases exclusive endpoint',ended:true,reopenedByAnotherProcess:true});
 const shared=await start(host,'pcm-48000',{backend:'wasapi',device:-1,channel:0,dsdMode:'pcm'});await host.control({session:shared.session,action:'play'});await sleep(100);assert.equal((await host.control({session:shared.session,action:'status'})).playing,true);await host.stopCurrent();
 const back=await start(host,'pcm-48000',settings);assert.equal(back.exclusive,true);await host.stopCurrent();report.checks.push({name:'shared to exclusive transition',recovered:true});report.ok=true;
 assert.equal(typeof device.endpointId,'string');assert(device.endpointId.length>0);
 const identified=await start(host,'pcm-44100',{...settings,wasapiDevice:255,wasapiDeviceId:device.endpointId});assert.equal(identified.exclusive,true);assert.equal(identified.wasapiDevice,device.id);await host.stopCurrent();
 await assert.rejects(start(host,'pcm-44100',{...settings,wasapiDeviceId:'unavailable-endpoint-identity'}),/WASAPI/);await host.stopCurrent();report.checks.push({name:'stable endpoint identity',staleNumericIndexIgnored:true,missingIdentityRejected:true});
}catch(error){report.ok=false;report.error=error.stack;throw error;}
finally{await Promise.allSettled([host.stopCurrent(),other.stopCurrent()]);host.close();other.close();await fs.writeFile(path.join(root,'build/wasapi-exclusive.json'),JSON.stringify(report,null,2));}
console.log(JSON.stringify(report,null,2));
