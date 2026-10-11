// Native application service: upstream transport plus explicit exclusive-output
// and repeat extensions. No HTML, DOM renderer, browser engine or WebView.
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { promises as fs } from 'node:fs';
import readline from 'node:readline';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { createHash,randomUUID } from 'node:crypto';
import {fixtureAlbums} from './demo-fixture.mjs';
import {cdAlbumForEject,performCdEject} from './cd-eject.mjs';
import {withSearchKeys} from './search-index.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const runtime=path.join(root,'runtime');
const appHome=path.resolve(process.env.RHINE_APP_HOME||path.join(process.env.APPDATA,'Rhine Music'));
const dataDir=path.resolve(process.env.MUSIC_DATA_DIR||path.join(appHome,'music-data'));
process.env.SONY_WALKMAN_ASSETS=path.join(runtime,'vendor/sony-walkman');
process.env.SONY_WALKMAN_PYTHON=path.join(runtime,'python/python.exe');
process.env.SONY_WALKMAN_X86_DLL=path.join(runtime,'vendor/sony-walkman-x86/sony-dsp-x86.dll');
process.env.SONY_WALKMAN_NATIVE_DECODER=path.join(runtime,'vendor/audio/windows-x64/rhine-audio-host.exe');
process.env.RHINE_NATIVE_AUDIO=process.env.SONY_WALKMAN_NATIVE_DECODER;
const nativeFetch=globalThis.fetch.bind(globalThis);let origin;
// Resolve remote audio at the original player's preparation/start boundary.
// This covers explicit play, automatic next, previous, seek restart and Sony
// setting changes while retaining the original loading/cancellation behavior.
globalThis.fetch=async(url,options)=>{
  const target=typeof url==='string'&&url.startsWith('/')?new URL(url,origin):new URL(url instanceof Request?url.url:url);
  if(target.origin===origin&&['/api/native/start','/api/sony/prepare'].includes(target.pathname)&&options?.method==='POST'){
    const args=JSON.parse(options.body);assertCdTrackId(args.trackId);if(typeof args.trackId==='string'&&args.trackId.startsWith('onlinetrack-'))await ensureOnlineId(args.trackId,options.signal);
    options.signal?.throwIfAborted();
  }
  return nativeFetch(url instanceof Request?new Request(target,url):target,options);
};
globalThis.document=new EventTarget();
// Original WASAPI/ASIO disables web ambience and interface sounds. Preserve
// that device-ownership rule and preferences, and reject browser playback.
globalThis.Audio=class NativeOnlyAmbience extends EventTarget {
  constructor(src=''){super();this.src=src;this.volume=0;this.paused=true;this.readyState=0;}
  play(){return Promise.reject(new Error('Native output owns the audio device.'));}
  pause(){this.paused=true;} load(){} removeAttribute(name){if(name==='src')this.src='';}
};
const send=value=>process.stdout.write(JSON.stringify(value)+'\n');
const log=(...values)=>process.stderr.write(values.join(' ')+'\n');console.log=log;console.warn=log;console.error=log;
const {MusicPlayer,setMusicAlbums,viewModel,isHiResAlbum,isHiResTrack,isDsdAlbum,isDsdTrack,normalizeOutput,normalizeSony,normalizeQuality,qualityPresets,demoAlbums,demoGenres,albumTitleParts}=await import('./original-core.mjs');
const {createMusicServer}=await import('../runtime/scripts/music-server.mjs');
const {MusicLibraryStore}=await import('../runtime/scripts/music-library.mjs');
const {OnlineLibrary}=await import('../runtime/scripts/online-sources.mjs');
const {FtpLibrary}=await import('../runtime/scripts/ftp-library.mjs');
const {SonyWalkman}=await import('../runtime/scripts/sony-walkman.mjs');
const {NativeOutput}=await import('../runtime/scripts/native-output.mjs');
await fs.mkdir(appHome,{recursive:true});const preferencePath=path.join(appHome,'preferences.json');
const defaults={repeatMode:'off',theme:'day',sortMode:'genre',quality:'original',reduced:false,volume:.65,songFade:true,sony:{dsee:false,dc:0,vinyl:0},sonyParallel:true,sonyStreaming:true,output:{backend:'wasapi',device:-1,channel:0,dsdMode:'pcm'},bgm:true,bgmVolume:.18,sound:true,soundVolume:.22,upscaler:'auto',upscaleQuality:'quality',graphicsApi:'auto',graphicsAdapter:'',graphicsSoftware:false,configurationMode:'automatic',reflections:'ssr',reflectionQuality:'balanced'};
const keys=new Set([...Object.keys(defaults),'renderQuality','archiveSaved']);let preferences={...defaults};
try{const saved=JSON.parse(await fs.readFile(preferencePath,'utf8'));for(const [key,value] of Object.entries(saved))if(keys.has(key))preferences[key]=value;}catch(error){if(error.code!=='ENOENT'&&!(error instanceof SyntaxError))throw error;}
function normalized(value){const result={...value};for(const [key,values]of Object.entries({repeatMode:['off','track','album'],theme:['day','night'],sortMode:['genre','artist','album'],quality:Object.keys(qualityPresets),configurationMode:['automatic','custom'],graphicsApi:['auto','dx11','dx12'],upscaler:['auto','native','dlss','fsr'],upscaleQuality:['quality','balanced','performance','native-aa'],reflections:['off','ssr','rtx'],reflectionQuality:['performance','balanced','high']}))if(!values.includes(result[key]))result[key]=defaults[key];for(const key of ['volume','soundVolume','bgmVolume'])result[key]=Number.isFinite(result[key])?Math.max(0,Math.min(1,result[key])):defaults[key];for(const key of ['reduced','songFade','bgm','sound','sonyParallel','sonyStreaming','graphicsSoftware'])result[key]=typeof result[key]==='boolean'?result[key]:defaults[key];if(typeof result.graphicsAdapter!=='string'||!/^([0-9a-fA-F]{16})?$/.test(result.graphicsAdapter))result.graphicsAdapter='';result.sony=normalizeSony(result.sony);result.output=normalizeOutput(result.output);if(result.output.backend==='browser')result.output={...defaults.output};result.renderQuality=normalizeQuality(result.renderQuality||qualityPresets[result.quality]);return result;}
preferences=normalized(preferences);
let saving=Promise.resolve();function save(){const text=JSON.stringify(preferences);saving=saving.catch(()=>{}).then(async()=>{await fs.writeFile(preferencePath+'.next',text,{mode:0o600});await fs.rename(preferencePath+'.next',preferencePath)});return saving;}
const store=await new MusicLibraryStore({dataDir}).init(),online=new OnlineLibrary({dataDir}),ftp=await new FtpLibrary({dataDir}).init();
let cdAlbums=[],cdFiles=new Map(),onlineFiles=new Map(),demoFiles=new Map(),cdSignature='',cdRefresh=null;
const cdEjecting=new Set();let cdGeneration=0,cdTransportStopping=false,cdReportReading=false,cdFixturePresent=false,cdFixtureDriveCount=1;
const downloads=new Map(),downloadAbort=new AbortController();
const combined=Object.create(store);combined.trackFile=id=>store.trackFile(id)??cdFiles.get(id)??onlineFiles.get(id)??demoFiles.get(id);
const sony=new SonyWalkman({dataDir,store:{trackFile:id=>combined.trackFile(id)??ftp.trackFile(id)}});
const native=new NativeOutput({store:combined,ftp,sony});
const service=await createMusicServer({dataDir,distDir:path.join(runtime,'public'),store,online,ftp,sony,native});
await new Promise((resolve,reject)=>{service.server.once('error',reject);service.server.listen(0,'127.0.0.1',resolve);});origin='http://127.0.0.1:'+service.server.address().port;
const player=new MusicPlayer({volume:preferences.volume,songFadeEnabled:preferences.songFade,sonySettings:preferences.sony,sonyExecution:{streaming:preferences.sonyStreaming,parallel:preferences.sonyParallel},outputSettings:preferences.output,bgmVolume:preferences.bgmVolume,bgmEnabled:preferences.bgm});
player.setRepeatMode(preferences.repeatMode);
player.subscribe(state=>{const {queue,...transport}=state;send({event:'player',state:transport,queueIds:queue.map(track=>track.id)});});
let demoCovers=false,shelf='local',library,libraryText='',closed=false,polling=false;
const request=async(route,body)=>{if(typeof route!=='string'||!route.startsWith('/api/')||route.includes('\\')||route.includes('\0'))throw new Error('Invalid local route');const response=await fetch(route,body===undefined?undefined:{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});const value=await response.json();if(!response.ok)throw new Error(value.error||'Local service request failed');return value;};
function markAlbum(album){return {...album,titleParts:albumTitleParts(album.title),hiRes:isHiResAlbum(album),dsd:isDsdAlbum(album),tracks:album.tracks.map(track=>({...track,hiRes:isHiResTrack(track),dsd:isDsdTrack(track)}))};}
async function refreshLibrary(force=false){
  const demo=process.env.RHINE_DEMO==='1'||demoCovers,snapshot=demo?{version:1,albums:demoAlbums.map(album=>({...album})),genres:demoGenres,roots:[],scan:{running:false},onlineEnabled:false}:await request(shelf==='online'?'/api/online/library':'/api/library');
  if(!demo&&shelf==='local'){snapshot.albums.push(...cdAlbums);if(cdAlbums.length)snapshot.genres.push({id:'audio-cd',name:'Audio CD'});}
  if(demo&&!demoCovers){snapshot.albums=await fixtureAlbums(snapshot.albums,appHome,demoFiles,cdFixturePresent,cdFixtureDriveCount);if(process.env.RHINE_DEMO_FIXTURE==='large')snapshot.genres=Array.from({length:9},(_,index)=>({id:'large-genre-'+index,name:'Synthetic genre '+index}));}
  snapshot.albums=snapshot.albums.map(markAlbum);const text=JSON.stringify(snapshot);if(!force&&text===libraryText)return;libraryText=text;library=snapshot;
  setMusicAlbums(snapshot.albums,snapshot.genres,preferences.sortMode);const model=viewModel();library.albums=model.albums.map(album=>withSearchKeys(album,snapshot.genres.find(genre=>genre.id===album.genreId)?.name||''));library.columns=model.columns;library.columnFiles=model.columnFiles;
  player.updateTrackMetadata(library.albums.flatMap(album=>album.tracks));send({event:'library',library,shelf});
}
const runFile=promisify(execFile);
function readingCd(){if(!cdReportReading){cdReportReading=true;send({event:'cd-reading',reading:true});}}
function refreshCd(visible=false){if(cdEjecting.size)return Promise.resolve();if(visible)readingCd();if(cdRefresh)return cdRefresh;cdRefresh=readCd().finally(()=>{cdRefresh=null;if(cdReportReading){cdReportReading=false;send({event:'cd-reading',reading:false});}});return cdRefresh;}
async function readCd(){
  const generation=cdGeneration;
  const result=await runFile(path.join(root,'RhineDeviceProbe.exe'),['--cd-only'],{windowsHide:true,encoding:'utf8',timeout:12000,maxBuffer:128*1024});
  const discs=JSON.parse(result.stdout).opticalDrives,signature=JSON.stringify(discs.map(disc=>[disc.drive,disc.discId,disc.error]));if(closed||generation!==cdGeneration||signature===cdSignature)return;
  const files=new Map(),albums=[];for(const disc of discs)if(!disc.error){
    const albumId='cd-'+createHash('sha256').update(disc.drive+'\0'+disc.discId).digest('hex').slice(0,32);
    const tracks=disc.tracks.filter(track=>track.audio).map(track=>{const id=albumId+'-'+track.number;files.set(id,{path:disc.drive+'\\Track'+String(track.number).padStart(2,'0')+'.cda',allowedRoot:disc.drive+'\\',mediaSignature:albumId});return {id,albumId,title:'Track '+String(track.number).padStart(2,'0'),artist:'Audio CD',trackNumber:track.number,discNumber:1,duration:track.duration,format:'CD-DA',codec:'PCM',bitsPerSample:16,sampleRate:44100,numberOfChannels:2,lossless:true,browserPlayable:false,audioUrl:'/api/audio/'+id,relativePath:'Track'+String(track.number).padStart(2,'0')+'.cda'};});
    if(tracks.length)albums.push({id:albumId,title:'Audio CD · '+disc.drive,artist:'Audio CD',genreId:'audio-cd',rawGenres:[],folder:disc.drive+'\\',tracks,producers:[],offline:false,cd:{drive:disc.drive,discId:disc.discId}});
  }
  if(albums.some(album=>!cdAlbums.some(old=>old.id===album.id)))readingCd();
  if(player.state.currentTrack?.id.startsWith('cd-')&&!files.has(player.state.currentTrack.id))player.stop();cdFiles=files;cdAlbums=albums;cdSignature=signature;send({event:'cd',discs});await refreshLibrary(true);
}
function assertCdTrackId(id){
  if(cdTransportStopping)throw new Error('正在停止 CD 播放，请稍后重试');
  const album=cdAlbums.find(album=>album.tracks.some(track=>track.id===id));
  if(album&&cdEjecting.has(album.cd.drive))throw new Error('CD 正在弹出，请稍后重试');
}
function assertCdTransport(){assertCdTrackId(player.state.currentTrack?.id);}
async function ejectCd(id){
  const album=cdAlbumForEject(cdAlbums,id),drive=album.cd.drive;
  if(cdEjecting.has(drive))throw new Error('CD 正在弹出，请稍后重试');
  cdEjecting.add(drive);++cdGeneration;
  try{
    await cdRefresh?.catch(()=>{});
    return await performCdEject(album,{
      current:()=>player.state.currentTrack,
      stop:async()=>{cdTransportStopping=true;try{player.stop();await native.stopCurrent();}finally{cdTransportStopping=false;}},
      release:drive=>native.command({cmd:'cd-release',drive}),
      eject:async(drive,discId)=>{
        try{const result=await runFile(path.join(root,'RhineDeviceProbe.exe'),['--eject-cd',drive,discId],{windowsHide:true,encoding:'utf8',timeout:12000,maxBuffer:128*1024});return JSON.parse(result.stdout);}
        catch(error){let reason;try{reason=JSON.parse(error.stderr||error.stdout||'{}').error;}catch{}throw new Error(reason||'无法弹出 CD，请检查光驱是否被其他程序占用');}
      },
      removed:async()=>{cdAlbums=cdAlbums.filter(value=>value.id!==album.id);for(const track of album.tracks)cdFiles.delete(track.id);cdSignature='';await refreshLibrary(true);}
    });
  }finally{cdEjecting.delete(drive);void refreshCd().catch(error=>send({event:'cd',error:error.message}));}
}
async function ensureOnlineId(id,signal){
  signal?.throwIfAborted();if(onlineFiles.has(id))return;
  const snapshot=await online.snapshot(),track=snapshot.albums.flatMap(album=>album.tracks).find(track=>track.id===id);
  if(!track)throw new Error('在线专辑架中没有这首曲目');
  let task=downloads.get(id);
  if(!task){const aborter=new AbortController();task={aborter,users:0,promise:null};task.promise=downloadOnline(track,AbortSignal.any([aborter.signal,downloadAbort.signal])).finally(()=>{if(downloads.get(id)===task)downloads.delete(id);});downloads.set(id,task);}
  ++task.users;
  try{
    if(!signal){await task.promise;return;}
    await new Promise((resolve,reject)=>{const abort=()=>{cleanup();reject(signal.reason||new DOMException('Playback cancelled','AbortError'));};const cleanup=()=>signal.removeEventListener('abort',abort);signal.addEventListener('abort',abort,{once:true});task.promise.then(value=>{cleanup();resolve(value);},error=>{cleanup();reject(error);});if(signal.aborted)abort();});
  }finally{if(--task.users===0&&downloads.get(id)===task){downloads.delete(id);task.aborter.abort();}}
}
async function downloadOnline(track,signal){
  const directory=path.join(dataDir,'native-online-cache');await fs.mkdir(directory,{recursive:true});
  const filename=path.join(directory,createHash('sha256').update(track.id+'\0'+track.audioUrl).digest('hex')+'.'+String(track.format||'audio').toLowerCase().replace(/[^a-z0-9]/g,''));
  // Cancellation and partial files are scoped to this request. Stream to disk
  // rather than retaining a song in RAM or imposing a new song-size limit.
  if(!await fs.stat(filename).then(s=>s.isFile()&&s.size>0).catch(()=>false)){
    signal.throwIfAborted();const response=await fetch(track.audioUrl,{signal});if(!response.ok||!response.body)throw new Error('在线音频读取失败：HTTP '+response.status);
    const partial=filename+'.'+randomUUID()+'.partial',file=await fs.open(partial,'wx');let size=0;
    try{for await(const chunk of response.body){signal.throwIfAborted();let offset=0;while(offset<chunk.length){const {bytesWritten}=await file.write(chunk,offset,chunk.length-offset);if(bytesWritten===0)throw new Error('无法写入在线音频缓存');offset+=bytesWritten;}size+=chunk.length;}if(size===0)throw new Error('在线音频为空');signal.throwIfAborted();await file.close();await fs.rename(partial,filename);}catch(error){await file.close().catch(()=>{});await fs.rm(partial,{force:true}).catch(()=>{});throw error;}
  }
  signal.throwIfAborted();
  onlineFiles.set(track.id,{path:filename,allowedRoot:directory});
}
async function preferenceUpdate(changes){
  if(!changes||typeof changes!=='object'||JSON.stringify(changes).length>8192)throw new Error('Invalid preferences');const before=preferences;
  preferences=normalized({...preferences,...Object.fromEntries(Object.entries(changes).filter(([key])=>keys.has(key))),...(changes.quality&&!changes.renderQuality?{renderQuality:qualityPresets[changes.quality]}:{})});
  await save();send({event:'preferences',preferences});player.setRepeatMode(preferences.repeatMode);player.setVolume(preferences.volume);player.setSongFadeEnabled(preferences.songFade);player.setBgmVolume(preferences.bgmVolume);player.setBgmEnabled(preferences.bgm);player.setSonyExecution({streaming:preferences.sonyStreaming,parallel:preferences.sonyParallel});
  if(JSON.stringify(before.output)!==JSON.stringify(preferences.output))await player.setOutputSettings(preferences.output);if(JSON.stringify(before.sony)!==JSON.stringify(preferences.sony))await player.setSonySettings(preferences.sony);if(before.sortMode!==preferences.sortMode)await refreshLibrary(true);return preferences;
}
async function dispatch(message){const args=message.args||{};switch(message.op){
  case 'archives':{const content=JSON.parse(await fs.readFile(path.join(runtime,'content/archives.json'),'utf8'));const albums=content.records.map(record=>({...record,tracks:[],artist:record.department,genreId:record.category,coverUrl:''}));return {albums,columns:content.columns,columnFiles:content.columns.map(category=>albums.flatMap((record,index)=>record.category===category?[index]:[])),genres:content.categories.map(name=>({id:name,name})),archive:true};}
  case 'state':return {preferences,library,player:player.state,shelf};
  case 'demo':demoCovers=true;await refreshLibrary(true);return {ok:true};
  case 'request':{if(args.path==='/api/library/scan')demoCovers=false;const result=await request(args.path,args.body);if(/\/library|\/albums|\/config|\/genre-rules/.test(args.path))await refreshLibrary(true);return result;}
  case 'preferences':return preferenceUpdate(args);
  case 'shelf':if(!['local','online'].includes(args.shelf))throw new Error('Invalid shelf');shelf=args.shelf;await refreshLibrary(true);return {shelf};
  case 'refresh':await refreshLibrary(true);return {ok:true};case 'cd:refresh':await refreshCd(true);return {albums:cdAlbums};
  case 'demo:cd':if(process.env.RHINE_DEMO!=='1'||process.env.RHINE_DEMO_FIXTURE!=='cd-media')throw new Error('Isolated CD fixture only');readingCd();try{cdFixturePresent=!!args.inserted;if(args.driveCount!==undefined){if(!Number.isInteger(args.driveCount)||args.driveCount<1||args.driveCount>3)throw new Error('Isolated fixture drive count must be 1 to 3');cdFixtureDriveCount=args.driveCount;}await refreshLibrary(true);}finally{cdReportReading=false;send({event:'cd-reading',reading:false});}return {inserted:cdFixturePresent};
  case 'cd:eject':return ejectCd(args.albumId);
  case 'play':{const albums=library?.albums||[],album=albums.find(a=>a.id===args.albumId),tracks=album?.tracks||albums.flatMap(a=>a.tracks),track=tracks.find(t=>t.id===args.trackId)||tracks[0];if(!track)throw new Error('曲库没有可播放曲目');assertCdTrackId(track.id);await player.play(track.id,tracks);return player.state;}
  case 'toggle':assertCdTransport();await player.toggle();return player.state;case 'next':assertCdTransport();await player.next();return player.state;case 'previous':assertCdTransport();await player.previous();return player.state;
  case 'seek':{assertCdTransport();const position=args.delta===undefined?Number(args.position):player.state.currentTime+Number(args.delta);if(!Number.isFinite(position))throw new Error('Invalid seek position');player.seek(position);return player.state;}
  case 'previous-track':{assertCdTransport();const state=player.state,track=state.queue[state.currentIndex-1];if(track)await player.play(track.id);else player.seek(0);return player.state;}
  case 'stop':player.stop();return player.state;
  case 'shutdown':await shutdown();return {ok:true};default:throw new Error('Unknown native command');
}}
async function shutdown(){if(closed)return;closed=true;clearInterval(poll);downloadAbort.abort();player.dispose();await Promise.allSettled([...downloads.values()].map(task=>task.promise));await native.stopCurrent().catch(()=>{});native.close();sony.close();ftp.close();await saving.catch(()=>{});service.server.closeAllConnections();await new Promise(resolve=>service.server.close(resolve));}
const lines=readline.createInterface({input:process.stdin,crlfDelay:Infinity});
lines.on('line',line=>{if(line.length>128*1024){void shutdown().finally(()=>process.exit(1));return;}let message;try{message=JSON.parse(line);if(!Number.isSafeInteger(message.id)||typeof message.op!=='string')throw new Error('Invalid RPC');}catch(error){send({event:'error',error:error.message});return;}void dispatch(message).then(result=>send({id:message.id,ok:true,result}),error=>send({id:message.id,ok:false,error:error.message}));});
lines.once('close',()=>void shutdown());process.once('SIGTERM',()=>void shutdown());let cdPoll=0;
const poll=setInterval(()=>{if(closed||polling)return;polling=true;void(async()=>{if(++cdPoll>=6){cdPoll=0;await refreshCd();}await refreshLibrary();})().catch(error=>send({event:'error',error:error.message})).finally(()=>{polling=false;});},500);
send({event:'ready',origin,preferences,backend:'native-d3d12'});await refreshLibrary(true);await refreshCd(process.env.RHINE_DEMO!=='1').catch(error=>send({event:'cd',error:error.message}));
