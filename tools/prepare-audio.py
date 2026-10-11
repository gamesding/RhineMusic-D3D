from pathlib import Path
import hashlib,json,shutil
root=Path(__file__).resolve().parent.parent
source=(root/'upstream/native/audio-host.cpp').read_text('utf-8')
shutil.copy2(root/'vendor/basscd/c/basscd.h',root/'runtime/vendor/audio/windows-x64/include/basscd.h')
expected='for(const wchar_t* name:{L"bassflac.dll",L"bassopus.dll"})'
if source.count(expected)!=1:raise RuntimeError('Unexpected original decoder source')
source=source.replace(expected,'for(const wchar_t* name:{L"bassflac.dll",L"bassopus.dll",L"basscd.dll"})')
def extend(before,after):
    global source
    if source.count(before)!=1:raise RuntimeError('Unexpected audio extension marker: '+before[:80])
    source=source.replace(before,after)
extend('#include "bassdsd.h"','#include "bassdsd.h"\n#define BASSWASAPIDEF(f) (WINAPI *f)\n#include "basswasapi.h"')
extend('#include "bassdsd.h"','#include "bassdsd.h"\n#define NOBASSOVERLOADS\n#define BASSCDDEF(f) (WINAPI *f)\n#include "basscd.h"')
extend('static json command(const json& value) {','#include "cd-release.inc"\nstatic json command(const json& value) {')
extend('if(cmd=="devices")return devices();','if(cmd=="cd-release")return releaseCd(value);\n if(cmd=="devices")return devices();')
extend('static void initialize() {','#include "wasapi-exclusive.inc"\nstatic void initialize() {')
extend('LOAD(dsd,BASS_DSD_StreamCreateFile);LOAD(dsd,BASS_DSD_StreamCreateFileUser);','LOAD(dsd,BASS_DSD_StreamCreateFile);LOAD(dsd,BASS_DSD_StreamCreateFileUser);initializeWasapi();')
extend('if(growing)growing->cancelled=true;','if(growing)growing->cancelled=true;closeWasapi();')
extend('backend!="asio"&&backend!="wasapi"','backend!="asio"&&backend!="wasapi"&&backend!="wasapi-exclusive"')
extend('if(mode!="pcm"&&mode!="native"&&mode!="dop")','if(backend=="wasapi-exclusive"&&mode!="pcm")throw std::runtime_error("WASAPI 独占使用 PCM；原生 DSD／DoP 请使用 ASIO");\n if(mode!="pcm"&&mode!="native"&&mode!="dop")')
extend('stream=openFile(request,mode,backend=="wasapi");','if(backend=="wasapi-exclusive"&&systemDevice>=0){check(BASS_SetDevice(systemDevice),"System device release failed");check(BASS_Free(),"System device release failed");systemDevice=-1;check(BASS_SetDevice(0),"Decoder device unavailable");}\n   stream=openFile(request,mode,backend=="wasapi");')
extend('const int device=request.at("device").get<int>();outputChannel=request.value("channel",0u);','''if(backend=="wasapi-exclusive") {
     float gain=request.value("volume",1.f);if(!std::isfinite(gain)||gain<0||gain>1)throw std::runtime_error("Invalid volume");exclusiveGain.store(gain);
     double position=request.value("position",0.);double duration=BASS_ChannelBytes2Seconds(stream,BASS_ChannelGetLength(stream,BASS_POS_BYTE));
     if(!std::isfinite(position)||position<0||position>duration)throw std::runtime_error("Invalid seek position");
     if(position>0)check(BASS_ChannelSetPosition(stream,BASS_ChannelSeconds2Bytes(stream,position),BASS_POS_BYTE),"Seek failed");
     openWasapiDevice(wasapiEndpoint(request));session=request.at("session").get<std::string>();pausedPosition=position;paused=true;ended=false;return status();
   }
   const int device=request.at("device").get<int>();outputChannel=request.value("channel",0u);''')
extend('{"asioVersion",BASS_ASIO_GetVersion()}', '{"asioVersion",BASS_ASIO_GetVersion()},{"wasapiVersion",BASS_WASAPI_GetVersion()},{"wasapiDevices",wasapiDevices()}')
extend('const double position=ended?duration:paused?pausedPosition:std::clamp','const double position=ended?duration:paused?pausedPosition:exclusive?exclusivePosition(duration):std::clamp')
extend('error="ASIO device changed or requested reset; select the device again";BASS_ASIO_Stop();','error=backend=="wasapi-exclusive"?"WASAPI 独占设备已断开或驱动停止；请重新选择输出设备":"ASIO device changed or requested reset; select the device again";if(asio)BASS_ASIO_Stop();closeWasapi();')
extend('if(dsdReadFailed)error="Native DSD source could not be read";','if(dsdReadFailed)error="Native DSD source could not be read";\n if(exclusiveReadFailed){error="WASAPI 独占无法读取音源";pausedPosition=position;paused=true;closeWasapi();}')
extend('50.0,1500.0):0;','50.0,1500.0):exclusive?std::clamp(50+2000.*exclusiveInfo.buflen/(exclusiveInfo.freq*exclusiveInfo.chans*sizeof(float)),50.,1500.):0;')
extend('if(asio)BASS_ASIO_Stop();}', 'if(asio)BASS_ASIO_Stop();closeWasapi();}')
extend('{"backend",backend},{"sampleRate",outputRate}', '{"backend",backend},{"exclusive",exclusive},{"wasapiInitFlags",exclusive?exclusiveInfo.initflags:0},{"wasapiFormat",exclusive?exclusiveInfo.format:0},{"wasapiDevice",exclusive?int(exclusiveDevice.load()):-1},{"sampleRate",outputRate}')
extend('else check(BASS_ChannelPlay(stream,FALSE),"System playback failed");','else if(backend=="wasapi-exclusive")playWasapi();else check(BASS_ChannelPlay(stream,FALSE),"System playback failed");')
extend('if(!paused)pausedPosition=BASS_ChannelBytes2Seconds(stream,BASS_ChannelGetPosition(stream,BASS_POS_BYTE));','if(backend=="wasapi-exclusive"){if(!paused)pauseWasapi();}else if(!paused)pausedPosition=BASS_ChannelBytes2Seconds(stream,BASS_ChannelGetPosition(stream,BASS_POS_BYTE));')
extend('else if(!paused)check(BASS_ChannelPause(stream),"System pause failed");','else if(backend!="wasapi-exclusive"&&!paused)check(BASS_ChannelPause(stream),"System pause failed");')
extend('const bool resume=!paused;if(asio&&BASS_ASIO_IsStarted())','const bool resume=!paused;if(exclusive){if(BASS_WASAPI_IsStarted())exclusiveCheck(BASS_WASAPI_Stop(TRUE)!=FALSE,"Exclusive stop before seek failed");paused=true;}if(asio&&BASS_ASIO_IsStarted())')
extend('if(resume&&asio){check(BASS_ASIO_Start(0,1),"ASIO resume after seek failed",true);paused=false;}','if(resume&&asio){check(BASS_ASIO_Start(0,1),"ASIO resume after seek failed",true);paused=false;}else if(resume&&backend=="wasapi-exclusive"){playWasapi();paused=false;}')
extend('else check(BASS_ChannelSetAttribute(stream,BASS_ATTRIB_VOL,gain),"System volume failed");','else if(backend=="wasapi-exclusive")exclusiveGain.store(gain);else check(BASS_ChannelSetAttribute(stream,BASS_ATTRIB_VOL,gain),"System volume failed");')
(root/'native/audio-host.cpp').write_text(source,encoding='utf-8')
vendor=root/'runtime/vendor/audio/windows-x64'
shutil.copy2(root/'vendor/basscd/x64/basscd.dll',vendor/'basscd.dll')
shutil.copy2(root/'vendor/basscd/basscd.txt',vendor/'licenses/basscd.txt')
manifest=json.loads((vendor/'assets.json').read_text('utf-8'))
manifest['files']['basscd.dll']=hashlib.sha256((vendor/'basscd.dll').read_bytes()).hexdigest()
manifest['files']['basswasapi.dll']=hashlib.sha256((vendor/'basswasapi.dll').read_bytes()).hexdigest()
manifest['sourceSha256']=hashlib.sha256((root/'native/audio-host.cpp').read_bytes()).hexdigest()
(vendor/'assets.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
transport=(root/'upstream/scripts/native-output.mjs').read_text('utf-8')
assert transport.count("['wasapi','asio']")==2
transport=transport.replace("['wasapi','asio']","['wasapi','wasapi-exclusive','asio']")
transport=transport.replace('return {backend:value.backend,device:value.device,channel:value.channel,dsdMode:value.dsdMode}', '''const wasapiDevice=value.wasapiDevice??-1;
 if(value.backend==='wasapi-exclusive'&&(!Number.isInteger(wasapiDevice)||wasapiDevice< -1||wasapiDevice>255))throw fail('无效的 WASAPI 输出设备');
 const wasapiDeviceId=value.wasapiDeviceId??'';
 if(value.backend==='wasapi-exclusive'&&(typeof wasapiDeviceId!=='string'||wasapiDeviceId.length>1024||wasapiDeviceId.includes('\\0')))throw fail('无效的 WASAPI 输出设备标识');
 return {backend:value.backend,device:value.device,channel:value.channel,dsdMode:value.dsdMode,...(value.backend==='wasapi-exclusive'?{wasapiDevice,wasapiDeviceId}:{})}''')
transport=transport.replace('devices:result.devices,backends:', 'devices:result.devices,wasapiDevices:result.wasapiDevices,backends:')
transport=transport.replace('backend:settings.backend,device:', "backend:settings.backend,wasapiDevice:settings.wasapiDevice??-1,wasapiDeviceId:settings.wasapiDeviceId??'',device:")
(root/'runtime/scripts/native-output.mjs').write_text(transport,'utf-8')
print('Added official CD-DA decoding and exclusive WASAPI output; Sony and ASIO paths retained.')
