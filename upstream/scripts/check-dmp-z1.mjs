// Original DMP-Z1 configuration: validate real DSP, process boundaries and cache identity.
import test from 'node:test'
import assert from 'node:assert/strict'
import {promises as fs} from 'node:fs'
import {createHash} from 'node:crypto'
import path from 'node:path'
import os from 'node:os'
import {SonyWalkman,sonySettings,sonyPlaybackPlan} from './sony-walkman.mjs'
import {normalizeSony,sonyEnabled} from '../src/sony-audio.ts'
import {silentWav} from './audio-fixtures.mjs'

const sha=bytes=>createHash('sha256').update(bytes).digest('hex')
const sleep=ms=>new Promise(resolve=>setTimeout(resolve,ms))
const settings={dsee:true,dc:2,vinyl:1}
const profile={...settings,dmpZ1:true}

test('DMP-Z1 settings preserve legacy defaults and reject non-boolean profiles',()=>{
  for(const value of [settings,{...settings,dmpZ1:false}]) {
    assert.deepEqual(sonySettings(value),settings)
    assert.deepEqual(normalizeSony(value),settings)
  }
  assert.deepEqual(sonySettings(profile),profile)
  assert.deepEqual(normalizeSony(profile),profile)
  for(const value of [0,1,'true',null,{}])assert.throws(()=>sonySettings({...settings,dmpZ1:value}))
  assert.throws(()=>sonySettings({...settings,dsdRemaster:true}))
  assert.equal(sonyEnabled({dsee:false,dc:0,vinyl:0,dmpZ1:true}),false)
})

test('DMP-Z1 configuration cannot enable DSEE at unsupported rates or on raw DSD',()=>{
  for(const rate of [96000,192000,2822400]) {
    const plan=sonyPlaybackPlan(profile,{sampleRate:rate,numberOfChannels:2})
    assert.equal(plan.effectiveSettings.dsee,false)
    assert.ok(plan.bypassed.includes('dsee'))
    assert.equal(plan.sourceSampleRate,rate)
  }
  assert.equal(sonyPlaybackPlan(profile,{sampleRate:44100,numberOfChannels:2}).effectiveSettings.dsee,true)
})

test('DMP-Z1 uses the original twenty-byte ICX1300 file and unchanged AI model',async()=>{
  const bytes=await fs.readFile('vendor/sony-walkman/DseeAi_HP_ICX1300.dcfg')
  assert.equal(bytes.length,20)
  assert.equal(sha(bytes),'396506ae7ff3bbf97f7d4f7aa9497f7ac898d2a06412fbe72000424ce96a312c')
  const old=await fs.readFile('vendor/sony-walkman/DseeAi.dcfg')
  assert.deepEqual(bytes.subarray(0,16),old.subarray(0,16))
  assert.equal(old.readUInt32LE(16),1)
  assert.equal(bytes.readUInt32LE(16),2)
  const manifest=JSON.parse(await fs.readFile('scripts/sony-walkman/assets.json','utf8'))
  assert.equal(sha(await fs.readFile('vendor/sony-walkman/DseeAi.bin')),manifest.files['DseeAi.bin'])
})

test('real DMP-Z1 x86 DSP is bit-identical with one, two or three continuous workers',
  {skip:process.platform!=='win32',timeout:120000},async t=>{
  const directory=await fs.mkdtemp(path.join(os.tmpdir(),'rhine-dmp-z1-'))
  const file=path.join(directory,'synthetic.wav'),input=silentWav(1.31,44100,24)
  let random=0x6a09e667
  for(let i=44,frame=0;i<input.length;i+=6,frame++) {
    random=(Math.imul(random,1664525)+1013904223)>>>0
    input.writeIntLE(Math.round(0x7fffff*(.18*Math.sin(frame*.067)+(random/4294967296-.5)*.015)),i,3)
    input.writeIntLE(Math.round(0x7fffff*(.13*Math.cos(frame*.039)-.04*Math.sin(frame*.171))),i+3,3)
  }
  await fs.writeFile(file,input)
  const sony=new SonyWalkman({dataDir:directory,store:{trackFile:()=>({path:file,allowedRoot:directory})}})
  t.after(async()=>{sony.close();while(sony.running)await sleep(20);await fs.rm(directory,{recursive:true,force:true})})
  const status=await sony.status()
  assert.equal(status.available,true,status.error)
  assert.equal(status.backend,'x86-cpp')
  assert.equal(status.dmpZ1Config,true)
  assert.equal(status.dsdRemaster,false)
  const results=[],audio=[]
  for(const [workers,dmpZ1] of [[1,true],[2,true],[3,true],[1,false]]) {
    const output=path.join(directory,`workers-${workers}-profile-${Number(dmpZ1)}.wav`),progress=[]
    const start=performance.now()
    const result=await sony.run(['--input',file,'--output',output,'--dsee','1','--dc','2','--vinyl','1',
      '--dmp-z1',String(Number(dmpZ1)),'--workers',String(workers),'--progressive'],
      value=>{if(value.state==='processing')progress.push({...value,elapsedMs:performance.now()-start})},60000)
    const bytes=await fs.readFile(output)
    assert.equal(result.frames,Math.round(1.31*44100)*4)
    assert.equal(bytes.length,56+result.frames*8)
    assert.equal(result.sampleRate,176400)
    assert.equal(result.workers,workers)
    assert.equal(result.dmpZ1,dmpZ1)
    assert.equal(result.dseeConfig,dmpZ1?'DseeAi_HP_ICX1300.dcfg':'DseeAi.dcfg')
    assert.equal(sha(bytes.subarray(56)),result.pcmSha256)
    assert.ok(result.peak>0)
    assert.ok(progress.some(p=>p.producedBytes>56),'progressive output must arrive before ready')
    for(let i=56;i<bytes.length;i+=4)assert.ok(Number.isFinite(bytes.readFloatLE(i)))
    audio.push(bytes)
    results.push({workers,dmpZ1,sampleRate:result.sampleRate,frames:result.frames,
      pcmSha256:result.pcmSha256,firstOutputMs:progress.find(p=>p.producedBytes>56).elapsedMs,
      renderSeconds:result.renderSeconds,dseeConfig:result.dseeConfig})
  }
  assert.deepEqual(audio[0],audio[1])
  assert.deepEqual(audio[0],audio[2])
  assert.notDeepEqual(audio[0],audio[3],'original ICX1300 selection must change actual PCM')
  async function completed(value) {
    const first=await sony.prepare('synthetic',value,{streaming:false,parallel:true})
    for(let attempt=0;attempt<1500;attempt++) {
      const current=sony.job(first.id)
      if(current.state==='error')throw new Error(current.error)
      if(current.state==='ready')return current
      await sleep(20)
    }
    throw new Error('DMP-Z1 cache test timed out')
  }
  const on=await completed(profile),off=await completed(settings),oldFalse=await completed({...settings,dmpZ1:false})
  assert.notEqual(on.id,off.id)
  assert.equal(off.id,oldFalse.id)
  assert.equal(on.pcmSha256,results[0].pcmSha256)
  assert.equal(off.pcmSha256,results[3].pcmSha256)
  assert.equal(sha(await fs.readFile(file)),sha(input),'effects must not modify the source')
  await fs.writeFile('docs/DMP-Z1-PIPELINE-VERIFICATION.json',JSON.stringify({date:new Date().toISOString(),
    source:'Sony DMP-Z1 1.02, original ICX1300 DSEE configuration',backend:status.backend,
    syntheticSource:true,bitExactBetweenWorkers:true,originalProfileChangesPcm:true,
    cacheProfilesSeparated:true,legacyFalsePreservesCacheIdentity:true,sourceUnmodified:true,
    dsdRemasterAvailable:false,results},null,2)+'\n')
})
