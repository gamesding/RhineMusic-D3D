import {promises as fs} from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {SonyWalkman,sonyPlaybackPlan} from './sony-walkman.mjs';

// Sustained tests are separate from the shorter ARM differential tests and
// run sequentially so simultaneous benchmark processes cannot bias timings.
const root=path.join(process.cwd(),'.tools/verification/sony-realtime'),seconds=30;
await fs.mkdir(root,{recursive:true});
function fixture(rate){
 const frames=rate*seconds,pcm=Buffer.alloc(frames*8);
 for(let i=0;i<frames;i++){
  const t=i/rate,envelope=.42+.18*Math.sin(2*Math.PI*.77*t);
  const l=envelope*(.17*Math.sin(2*Math.PI*431*t)+.09*Math.sin(2*Math.PI*7919*t)+.025*Math.sin(2*Math.PI*18001*t));
  const r=envelope*(.15*Math.sin(2*Math.PI*673*t)+.085*Math.sin(2*Math.PI*11573*t)+.019*Math.sin(2*Math.PI*19309*t));
  pcm.writeFloatLE(l,i*8);pcm.writeFloatLE(r,i*8+4);
 }
 const header=Buffer.alloc(56);header.write('RIFF');header.writeUInt32LE(48+pcm.length,4);header.write('WAVEfmt ',8);header.writeUInt32LE(16,16);header.writeUInt16LE(3,20);header.writeUInt16LE(2,22);header.writeUInt32LE(rate,24);header.writeUInt32LE(rate*8,28);header.writeUInt16LE(8,32);header.writeUInt16LE(32,34);header.write('fact',36);header.writeUInt32LE(4,40);header.writeUInt32LE(frames,44);header.write('data',48);header.writeUInt32LE(pcm.length,52);
 return Buffer.concat([header,pcm]);
}
const sony=new SonyWalkman({dataDir:root,store:{trackFile:()=>null}}),status=await sony.status();
assert.equal(status.available,true,status.error);assert.equal(status.backend,'x86-cpp');
const results=[];
try{
 for(const rate of [44100,48000,192000]){
  const input=path.join(root,`input-${rate}.wav`),output=path.join(root,'temporary-output.wav');
  await fs.writeFile(input,fixture(rate));
  const configs=[];
  if(rate<=48000){
   for(let dc=1;dc<=6;dc++)for(let vinyl=1;vinyl<=4;vinyl++)configs.push({dsee:1,dc,vinyl});
  }else{
   for(let dc=1;dc<=6;dc++)configs.push({dsee:1,dc,vinyl:0});
   for(let vinyl=1;vinyl<=4;vinyl++)configs.push({dsee:1,dc:0,vinyl});
   for(let dc=1;dc<=6;dc++)for(let vinyl=1;vinyl<=4;vinyl++)configs.push({dsee:1,dc,vinyl});
  }
  for(const config of configs){
   const plan=sonyPlaybackPlan({...config,dsee:Boolean(config.dsee)},{sampleRate:rate,numberOfChannels:2}),effective=plan.effectiveSettings;
   assert.equal(effective.dsee,rate<=48000);assert.equal(effective.dc,config.dc);assert.equal(effective.vinyl,config.vinyl);
   const start=performance.now(),progress=[],expectedRate=rate*(effective.dsee?4:1);
   const result=await sony.run(['--input',input,'--output',output,'--dsee',String(Number(effective.dsee)),'--dc',String(effective.dc),'--vinyl',String(effective.vinyl),'--workers','3','--progressive'],p=>{if(p.state==='processing')progress.push({...p,elapsedMs:performance.now()-start})},180000);
   assert.equal(result.sampleRate,expectedRate);assert.equal(result.frames,expectedRate*seconds);assert.ok(Number.isFinite(result.peak)&&result.peak>0);
   const realtimeFactor=seconds/result.renderSeconds;
   assert.ok(realtimeFactor>1,`Slower than realtime at ${rate}, DC ${config.dc}, Vinyl ${config.vinyl}`);
   const first=progress.find(p=>p.producedBytes>=expectedRate*8*.5+56);
   const row={inputRate:rate,seconds,...config,dseeActive:effective.dsee,bypassed:plan.bypassed,outputRate:result.sampleRate,frames:result.frames,workers:result.workers,renderSeconds:result.renderSeconds,realtimeFactor,firstHalfSecondBufferMs:first?.elapsedMs};
   results.push(row);console.log(JSON.stringify(row));
   await fs.writeFile(path.join(root,'progress.json'),JSON.stringify({date:new Date().toISOString(),results},null,2)+'\n');
   await fs.unlink(output);
  }
 }
 const report={date:new Date().toISOString(),source:'Generated stereo multitone, 30 seconds per case',backend:'x86-cpp',cases:results.length,passed:results.length,allFasterThanRealtime:true,minRealtimeFactor:Math.min(...results.map(r=>r.realtimeFactor)),measurement:'DSP pipeline including initialization; first buffer includes process startup, excludes audio-device latency',allPresetCombinationsAt44100And48000:true,allDcAndVinylPresetsAndCombinationsAt192000:true,hardwareRemasterVerified:false,machineTimbreVerified:false,results};
 await fs.writeFile('docs/SONY-REALTIME-VERIFICATION.json',JSON.stringify(report,null,2)+'\n');
 console.log(JSON.stringify({ok:true,cases:report.cases,minRealtimeFactor:report.minRealtimeFactor}));
}finally{sony.close()}
