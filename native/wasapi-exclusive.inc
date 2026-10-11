// Exclusive WASAPI extends the original BASS decoder. Sony and ASIO retain
// their original paths. Callback work is bounded and never changes OS volume.
static bool exclusive=false;
static int exclusiveEndpoint=-1;
static BASS_WASAPI_INFO exclusiveInfo{};
static DWORD exclusiveSourceChannels=2;
static std::atomic<DWORD> exclusiveDevice{DWORD(-1)};
static std::atomic<float> exclusiveGain{1};
static std::atomic<bool> exclusiveReadFailed{false};
static void exclusiveFailure(int code,const std::string& operation){
 std::string reason;
 switch(code){case BASS_ERROR_BUSY:reason="设备正在被其他程序占用";break;case BASS_ERROR_FORMAT:reason="设备不支持该采样率或声道格式";break;case BASS_ERROR_NOTAVAIL:reason="设备未允许独占输出";break;case BASS_ERROR_DEVICE:reason="输出设备已断开或无效";break;default:reason="驱动无法启动独占输出";}
 throw std::runtime_error(std::string("WASAPI 独占：")+reason+"；"+operation+" ("+std::to_string(code)+")");
}
static void exclusiveCheck(bool ok,const char* operation){if(!ok)exclusiveFailure(BASS_ErrorGetCode(),operation);}
static void CALLBACK exclusiveNotify(DWORD event,DWORD device,void*){
 if(device==exclusiveDevice.load()&&(event==BASS_WASAPI_NOTIFY_FAIL||event==BASS_WASAPI_NOTIFY_DISABLED))driverEvent.store(1);
}
static DWORD CALLBACK exclusiveOutput(void* buffer,DWORD length,void*){
 const DWORD channels=exclusiveInfo.chans,requested=length*exclusiveSourceChannels/channels;
 DWORD received=BASS_ChannelGetData(stream,buffer,requested);
 if(received==DWORD(-1)){if(BASS_ErrorGetCode()!=BASS_ERROR_ENDED)exclusiveReadFailed=true;return 0;}
 if(received>requested||received%(exclusiveSourceChannels*sizeof(float))){exclusiveReadFailed=true;return 0;}
 auto data=static_cast<float*>(buffer);DWORD samples=received/sizeof(float);
 if(exclusiveSourceChannels==1&&channels==2){for(DWORD i=samples;i>0;--i)data[2*i-1]=data[2*i-2]=data[i-1];samples*=2;}
 const float gain=exclusiveGain.load();if(gain!=1)for(DWORD i=0;i<samples;++i)data[i]*=gain;
 return samples*sizeof(float);
}
static void initializeWasapi(){
 const auto dll=library(L"basswasapi.dll");
 LOAD(dll,BASS_WASAPI_CheckFormat);LOAD(dll,BASS_WASAPI_GetVersion);LOAD(dll,BASS_WASAPI_GetDeviceInfo);LOAD(dll,BASS_WASAPI_Init);LOAD(dll,BASS_WASAPI_Free);
 LOAD(dll,BASS_WASAPI_GetInfo);LOAD(dll,BASS_WASAPI_Start);LOAD(dll,BASS_WASAPI_Stop);LOAD(dll,BASS_WASAPI_IsStarted);
 LOAD(dll,BASS_WASAPI_GetData);LOAD(dll,BASS_WASAPI_GetDevice);LOAD(dll,BASS_WASAPI_SetNotify);
 if(HIWORD(BASS_WASAPI_GetVersion())!=0x204)throw std::runtime_error("Unexpected BASSWASAPI version");
 exclusiveCheck(BASS_WASAPI_SetNotify(exclusiveNotify,nullptr)!=FALSE,"Device notification setup failed");
}
static json wasapiDevices(){
 json list=json::array();BASS_WASAPI_DEVICEINFO info{};
 for(DWORD i=0;i<256&&BASS_WASAPI_GetDeviceInfo(i,&info);++i)if(!(info.flags&(BASS_DEVICE_INPUT|BASS_DEVICE_LOOPBACK)))
  list.push_back({{"id",i},{"endpointId",info.id?info.id:""},{"name",info.name?info.name:""},{"enabled",bool(info.flags&BASS_DEVICE_ENABLED)},{"default",bool(info.flags&BASS_DEVICE_DEFAULT)},{"output",true},{"mixSampleRate",info.mixfreq},{"mixChannels",info.mixchans}});
 return list;
}
static int wasapiEndpoint(const json& request){
 const auto identity=request.value("wasapiDeviceId",std::string());if(identity.empty())return request.value("wasapiDevice",-1);
 if(identity.size()>1024||identity.find('\0')!=std::string::npos)throw std::runtime_error("WASAPI 独占：输出设备标识无效");
 BASS_WASAPI_DEVICEINFO info{};for(DWORD i=0;i<256&&BASS_WASAPI_GetDeviceInfo(i,&info);++i)if(info.id&&identity==info.id&&!(info.flags&(BASS_DEVICE_INPUT|BASS_DEVICE_LOOPBACK)))return int(i);
 throw std::runtime_error("WASAPI 独占：所选输出设备已断开，请重新连接或选择设备");
}
static void closeWasapi(){
 exclusiveDevice.store(DWORD(-1));if(exclusive){BASS_WASAPI_Stop(TRUE);BASS_WASAPI_Free();exclusive=false;}exclusiveReadFailed=false;
}
static std::string exclusiveFormatContext(int endpoint,DWORD rate,DWORD channels,bool supportedRates){
 BASS_WASAPI_DEVICEINFO info{};int resolved=endpoint;
 if(resolved==-1){for(DWORD i=0;i<256&&BASS_WASAPI_GetDeviceInfo(i,&info);++i)if((info.flags&(BASS_DEVICE_ENABLED|BASS_DEVICE_DEFAULT))==(BASS_DEVICE_ENABLED|BASS_DEVICE_DEFAULT)&&!(info.flags&(BASS_DEVICE_INPUT|BASS_DEVICE_LOOPBACK))){resolved=int(i);break;}}
 std::string name="所选输出设备";if(resolved>=0&&BASS_WASAPI_GetDeviceInfo(resolved,&info)&&info.name)name=info.name;
 std::string context=name+" · "+std::to_string(rate)+" Hz / "+std::to_string(channels)+" 声道";
 if(supportedRates){std::string rates;for(DWORD candidate:{44100u,48000u,88200u,96000u,176400u,192000u,352800u,384000u})if(BASS_WASAPI_CheckFormat(resolved,candidate,channels,BASS_WASAPI_EXCLUSIVE)!=DWORD(-1)){if(!rates.empty())rates+="、";rates+=std::to_string(candidate);}
  if(!rates.empty())context+="；设备支持 "+rates+" Hz";
  context+="。请在设置的“独占输出设备”选择支持此格式的 DAC，或使用“系统输出 · WASAPI”。";
 }
 return context;
}
static void openWasapiDevice(int endpoint){
 if(exclusive)return;
 if(endpoint< -1||endpoint>255)throw std::runtime_error("Invalid WASAPI output device");
 BASS_CHANNELINFO source{};exclusiveCheck(BASS_ChannelGetInfo(stream,&source)!=FALSE,"Source format unavailable");
 exclusiveSourceChannels=source.chans;exclusiveEndpoint=endpoint;exclusiveReadFailed=false;driverEvent=0;
 DWORD channels=source.chans,flags=BASS_WASAPI_EXCLUSIVE|BASS_WASAPI_EVENT|BASS_WASAPI_ASYNC;
 auto init=[&]{return BASS_WASAPI_Init(endpoint,source.freq,channels,flags,.05f,0,exclusiveOutput,nullptr)!=FALSE;};
 bool opened=init();
 if(!opened&&BASS_ErrorGetCode()==BASS_ERROR_FORMAT&&channels==1){channels=2;opened=init();}
 if(!opened&&BASS_ErrorGetCode()==BASS_ERROR_NOTAVAIL){flags=BASS_WASAPI_EXCLUSIVE;opened=init();}
 if(!opened){const int code=BASS_ErrorGetCode();exclusiveFailure(code,exclusiveFormatContext(endpoint,source.freq,channels,code==BASS_ERROR_FORMAT));}exclusive=true;
 try{
  exclusiveCheck(BASS_WASAPI_GetInfo(&exclusiveInfo)!=FALSE,"Exclusive format unavailable");
  if(!(exclusiveInfo.initflags&BASS_WASAPI_EXCLUSIVE)||exclusiveInfo.freq!=source.freq||exclusiveInfo.chans!=channels)throw std::runtime_error("WASAPI exclusive format changed; automatic resampling is disabled");
  exclusiveDevice.store(BASS_WASAPI_GetDevice());outputRate=exclusiveInfo.freq;
 }catch(...){closeWasapi();throw;}
}
static double exclusivePosition(double duration){
 double position=BASS_ChannelBytes2Seconds(stream,BASS_ChannelGetPosition(stream,BASS_POS_BYTE));
 if(exclusive){DWORD buffered=BASS_WASAPI_GetData(nullptr,BASS_DATA_AVAILABLE);if(buffered!=DWORD(-1))position-=double(buffered)/(exclusiveInfo.freq*exclusiveInfo.chans*sizeof(float));}
 return std::clamp(position,0.,duration);
}
static void playWasapi(){openWasapiDevice(exclusiveEndpoint);if(!BASS_WASAPI_IsStarted())exclusiveCheck(BASS_WASAPI_Start()!=FALSE,"Exclusive playback failed");}
static void pauseWasapi(){
 if(!exclusive)return;
 double duration=BASS_ChannelBytes2Seconds(stream,BASS_ChannelGetLength(stream,BASS_POS_BYTE));
 pausedPosition=exclusivePosition(duration);if(BASS_WASAPI_IsStarted())exclusiveCheck(BASS_WASAPI_Stop(TRUE)!=FALSE,"Exclusive pause failed");
 exclusiveCheck(BASS_ChannelSetPosition(stream,BASS_ChannelSeconds2Bytes(stream,pausedPosition),BASS_POS_BYTE)!=FALSE,"Exclusive pause position failed");
}
