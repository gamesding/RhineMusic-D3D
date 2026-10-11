from pathlib import Path
import argparse
p=argparse.ArgumentParser();p.add_argument('--target',required=True);args=p.parse_args();root=Path(args.target).resolve()
# Extend only transport metadata. Original Sony algorithms and runtime assets
# remain byte-identical, with their original hash checks still enforced.
source=(root/'upstream/scripts/read-audio-metadata.mjs').read_text('utf-8')
marker='export async function readAudioMetadata(filename,options={}) {'
if source.count(marker)!=1:raise ValueError('Upstream metadata entry changed')
source=source.replace(marker,marker+'''
  if(path.extname(filename).toLowerCase()==='.cda') {
    const {spawn}=await import('node:child_process');
    const executable=process.env.SONY_WALKMAN_NATIVE_DECODER;
    if(!executable)throw new Error('CD metadata requires the packaged native decoder');
    const value=await new Promise((resolve,reject)=>{
      const child=spawn(executable,['--file-info-json'],{windowsHide:true,stdio:['pipe','pipe','ignore']});
      let output='';const timer=setTimeout(()=>{child.kill();reject(new Error('CD metadata timed out'))},12000);
      child.once('error',error=>{clearTimeout(timer);reject(error)});
      child.stdout.on('data',chunk=>{output+=chunk;if(output.length>65536){child.kill();reject(new Error('CD metadata exceeds limit'))}});
      child.once('exit',code=>{clearTimeout(timer);try{const value=JSON.parse(output);if(code!==0||value.error)throw new Error(value.error||'CD metadata failed');resolve(value)}catch(error){reject(error)}});
      child.stdin.end(JSON.stringify({file:filename})+'\\n');
    });
    if(value.sampleRate!==44100||value.channels!==2||!Number.isFinite(value.duration)||value.duration<=0)throw new Error('Invalid CD-DA metadata');
    return {format:{sampleRate:44100,numberOfChannels:2,duration:value.duration,bitsPerSample:16,lossless:true,codec:'PCM',container:'CD-DA'},common:{},native:{},quality:{warnings:[]}};
  }
''')
(root/'runtime/scripts/read-audio-metadata.mjs').write_text(source,'utf-8')
source=(root/'upstream/scripts/sony-walkman.mjs').read_text('utf-8');marker='signature: `${source}:${stat.size}:${stat.mtimeMs}`'
if source.count(marker)!=1:raise ValueError('Upstream source signature changed')
source=source.replace(marker,'signature: `${source}:${stat.size}:${stat.mtimeMs}${descriptor.mediaSignature ? `:${descriptor.mediaSignature}` : ""}`')
(root/'runtime/scripts/sony-walkman.mjs').write_text(source,'utf-8')
print('Extended CD-DA metadata and removable-disc cache identity; Sony DSP unchanged.')
