import {promises as fs} from 'node:fs';
import path from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const {build}=await import(pathToFileURL(path.join(root,'build-tools/node_modules/esbuild/lib/main.js')).href);
const source=path.join(root,'upstream/src'),absolute=value=>JSON.stringify(path.join(source,value).replaceAll('\\','/'));
const code=`export {MusicPlayer} from ${absolute('music-player.ts')};
export {normalizeSony} from ${absolute('sony-audio.ts')};
export {normalizeOutput} from ${absolute('native-audio.ts')};
export {normalizeQuality,qualityPresets} from ${absolute('render-quality.ts')};
export {demoAlbums,demoGenres} from ${absolute('demo-library.ts')};
export {albumTitleParts} from ${absolute('music-title.ts')};
export {isHiResAlbum,isHiResTrack,isDsdAlbum,isDsdTrack} from ${absolute('hi-res.ts')};
export {setMusicAlbums} from ${absolute('data.ts')};
import {records,archiveColumns,columnFiles} from ${absolute('data.ts')};
export function viewModel(){return {albums:records.map(r=>r.album),columns:archiveColumns,columnFiles:archiveColumns.map((_,i)=>columnFiles(i))};}`;
const replaceOnce=(text,before,after)=>{if(text.split(before).length!==2)throw new Error('Upstream extension marker changed: '+before);return text.replace(before,after);};
const nativeExtensions={name:'native-output-and-repeat',setup(build){build.onLoad({filter:/(native-audio|music-player)\.ts$/},async({path:file})=>{
    let contents=await fs.readFile(file,'utf8');
    if(file.endsWith('native-audio.ts')){
        if(contents.split("'wasapi','asio'").length!==3)throw new Error('Upstream native backend lists changed');
        contents=contents.replaceAll("'wasapi','asio'","'wasapi','wasapi-exclusive','asio'");
        contents=replaceOnce(contents,"'browser'|'wasapi'|'asio'","'browser'|'wasapi'|'wasapi-exclusive'|'asio'");
        contents=replaceOnce(contents,"dsdMode: 'pcm'|'native'|'dop' }","dsdMode: 'pcm'|'native'|'dop'; wasapiDevice?: number; wasapiDeviceId?: string }");
        contents=replaceOnce(contents,"value!.dsdMode!:'pcm'};","value!.dsdMode!:'pcm',...(value?.backend==='wasapi-exclusive'?{wasapiDevice:Number.isInteger(value.wasapiDevice)&&value.wasapiDevice!>=-1&&value.wasapiDevice!<=255?value.wasapiDevice!:-1,wasapiDeviceId:typeof value.wasapiDeviceId==='string'&&value.wasapiDeviceId.length<=1024&&!value.wasapiDeviceId.includes('\\0')?value.wasapiDeviceId:''}:{})};");
    }else{
        contents=`import {repeatSuccessor,type RepeatMode} from ${JSON.stringify(path.join(root,'backend/repeat.ts'))};\n`+contents;
        contents=replaceOnce(contents,'export class MusicPlayer {',"export class MusicPlayer {\n  private repeatMode: RepeatMode = 'off';\n  setRepeatMode(mode:RepeatMode):void {this.repeatMode=['off','track','album'].includes(mode)?mode:'off';this.emit();}\n");
        contents=replaceOnce(contents,'export interface MusicPlayerState {','export interface MusicPlayerState {\n  repeatMode?: RepeatMode;');
        contents=replaceOnce(contents,'return { ...this.value, transport:', 'return { ...this.value, repeatMode:this.repeatMode, transport:');
        contents=replaceOnce(contents,'const next = this.value.queue[this.value.currentIndex + 1];','const next = repeatSuccessor(this.value.queue,this.value.currentIndex,this.repeatMode);');
        contents=replaceOnce(contents,'const track = this.value.queue[this.value.currentIndex + 1];',"const track = repeatSuccessor(this.value.queue,this.value.currentIndex,this.repeatMode==='album'?'album':'off');");
    }
    return {contents,loader:'ts'};
});}};
await build({nodePaths:[path.join(root,'build-tools/node_modules')],stdin:{contents:code,resolveDir:root,loader:'ts'},outfile:path.join(root,'backend/original-core.mjs'),bundle:true,platform:'node',format:'esm',target:'node26',legalComments:'inline',define:{'import.meta.env.BASE_URL':'"/"'},plugins:[nativeExtensions]});
const lock=JSON.parse(await fs.readFile(path.join(root,'upstream/package-lock.json'),'utf8'));
for(const [relative,details]of Object.entries(lock.packages))if(relative.startsWith('node_modules/')&&!details.dev){const from=path.join(root,'build-tools',relative),to=path.join(root,'runtime',relative);await fs.mkdir(path.dirname(to),{recursive:true});await fs.cp(from,to,{recursive:true});}
console.log('Bundled upstream transport with isolated WASAPI-exclusive and repeat extensions; upstream files unchanged.');

await fs.mkdir(path.join(root,"runtime/content"),{recursive:true});await fs.copyFile(path.join(root,"upstream/content/archives.json"),path.join(root,"runtime/content/archives.json"));
