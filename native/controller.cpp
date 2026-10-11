#include "controller.hpp"
#include <Xinput.h>
#include <cmath>
#include <algorithm>
namespace rhine {
static constexpr WORD xboxStrong=42000,xboxWeak=26000;
static constexpr uint8_t sonyStrong=160,sonyWeak=104;
Controllers::Controllers(){
    const auto path=executableDirectory()/L"runtime/drivers/rhine-pad-bridge.dll";
    bridge_=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if(!bridge_)throw std::runtime_error("Cannot load the bundled controller bridge: "+std::to_string(GetLastError()));
    try{
        auto resolve=[&](const char* name){auto symbol=GetProcAddress(bridge_,name);if(!symbol)throw std::runtime_error(std::string("Controller bridge entry missing: ")+name);return symbol;};
        const auto version=reinterpret_cast<decltype(&rhine_pad_api_version)>(resolve("rhine_pad_api_version"));
        if(version()!=RHINE_PAD_API_VERSION)throw std::runtime_error("Controller bridge ABI version mismatch");
        padInitialize_=reinterpret_cast<decltype(padInitialize_)>(resolve("rhine_pad_initialize"));
        padShutdown_=reinterpret_cast<decltype(padShutdown_)>(resolve("rhine_pad_shutdown"));
        padOpen_=reinterpret_cast<decltype(padOpen_)>(resolve("rhine_pad_open"));
        padClose_=reinterpret_cast<decltype(padClose_)>(resolve("rhine_pad_close"));
        padReadState_=reinterpret_cast<decltype(padReadState_)>(resolve("rhine_pad_read"));
        padVibrate_=reinterpret_cast<decltype(padVibrate_)>(resolve("rhine_pad_vibrate"));
        initialized_=padInitialize_()>=0;
    }catch(...){FreeLibrary(bridge_);bridge_=nullptr;throw;}
}
Controllers::~Controllers(){
    for(size_t i=0;i<8;++i)if(vibrationUntil_[i]>0)vibration(i,false);
    if(initialized_){for(int handle:handles_)if(handle>=0)padClose_(handle);padShutdown_();}
    if(bridge_)FreeLibrary(bridge_);
}
void Controllers::accept(size_t slot,unsigned buttons,float x,float y,double now,bool focused,const std::function<void(Action)>& emit){
    // Unified logical buttons; held inputs while unfocused never become edges.
    unsigned changed=buttons&~buttons_[slot];buttons_[slot]=buttons;
    int direction=0;if(std::max(std::abs(x),std::abs(y))>.35f)direction=std::abs(x)>std::abs(y)?(x<0?3:4):(y<0?1:2);
    if(buttons&1)direction=1;else if(buttons&2)direction=2;else if(buttons&4)direction=3;else if(buttons&8)direction=4;
    bool trigger=direction&&(directions_[slot]!=direction||now>=nextRepeat_[slot]);
    if(direction!=directions_[slot])nextRepeat_[slot]=now+.38;else if(trigger)nextRepeat_[slot]=now+.12;
    directions_[slot]=direction;
    if(!focused)return;
    if(trigger||changed)activeSlot_=int(slot);
    if(trigger)emit(direction==1?Action::up:direction==2?Action::down:direction==3?Action::left:Action::right);
    for(auto [mask,action]:{std::pair{16u,Action::confirm},{32u,Action::back},{64u,Action::faceLeft},{128u,Action::settings},{256u,Action::previous},{512u,Action::next},{1024u,Action::search},{2048u,Action::tab},{4096u,Action::library}})if(changed&mask)emit(action);
}
void Controllers::feedback(double now,bool focused){
    for(size_t i=0;i<8;++i)if(vibrationUntil_[i]>0&&(now>=vibrationUntil_[i]||!focused)){int result=vibration(i,false);vibrationUntil_[i]=0;++vibrationStops_;for(auto& item:vibrationResults_)if(item.value("slot",size_t(8))==i)item["stopResult"]=result;}
}
void Controllers::poll(double now,bool focused,const std::function<void(Action)>& emit,const std::function<void(float,float)>& analog){
    feedback(now,focused);
    const float dt=float(std::clamp(lastPoll_?now-lastPoll_:0.,0.,.05));lastPoll_=now;
    auto axis=[](float value){return std::copysign(std::max(0.f,(std::abs(value)-.16f)/.84f),value);};
    auto move=[&](size_t slot,float rotate,float scroll){if(!focused||!analog||(std::abs(rotate)<.00001f&&std::abs(scroll)<.01f))return;activeSlot_=int(slot);analog(rotate,scroll);};
    if(initialized_&&now>=nextOpen_){nextOpen_=now+1;for(size_t i=0;i<handles_.size();++i)if(handles_[i]<0)handles_[i]=padOpen_(uint32_t(i));}
    for(size_t i=0;i<4;++i){XINPUT_STATE data{};unsigned mapped=0;float x=0,y=0;if(XInputGetState(DWORD(i),&data)==ERROR_SUCCESS){names_[i]="Xbox / XInput";auto b=data.Gamepad.wButtons;if(b&XINPUT_GAMEPAD_DPAD_UP)mapped|=1;if(b&XINPUT_GAMEPAD_DPAD_DOWN)mapped|=2;if(b&XINPUT_GAMEPAD_DPAD_LEFT)mapped|=4;if(b&XINPUT_GAMEPAD_DPAD_RIGHT)mapped|=8;if(b&XINPUT_GAMEPAD_A)mapped|=16;if(b&XINPUT_GAMEPAD_B)mapped|=32;if(b&XINPUT_GAMEPAD_X)mapped|=64;if(b&XINPUT_GAMEPAD_START)mapped|=128;if(b&XINPUT_GAMEPAD_LEFT_SHOULDER)mapped|=256;if(b&XINPUT_GAMEPAD_RIGHT_SHOULDER)mapped|=512;if(b&XINPUT_GAMEPAD_Y)mapped|=1024;if(b&XINPUT_GAMEPAD_BACK)mapped|=4096;if(b&XINPUT_GAMEPAD_RIGHT_THUMB)mapped|=2048;x=std::clamp(data.Gamepad.sThumbLX/32767.f,-1.f,1.f);y=-std::clamp(data.Gamepad.sThumbLY/32767.f,-1.f,1.f);move(i,axis(std::clamp(data.Gamepad.sThumbRX/32767.f,-1.f,1.f))*dt*2.8f,axis(-std::clamp(data.Gamepad.sThumbRY/32767.f,-1.f,1.f))*dt*800.f);}else names_[i].clear();accept(i,mapped,x,y,now,focused,emit);}
    for(size_t i=0;i<4;++i){
        unsigned mapped=0;float x=0,y=0;bool connected=false,pressed=false;std::optional<float> touch;
        if(handles_[i]>=0){
            RhinePadSample data{};int result=padReadState_(handles_[i],&data,sizeof(data));
            if(result>=0&&data.connected){
                connected=true;
                names_[i+4]=data.generation==5?"DualSense":"DualShock 4";
                touchWidth_[i]=data.touchWidth;
                padRead_[i]={data.diagnosticButtons,data.diagnosticAxes[0],data.diagnosticAxes[1],data.diagnosticAxes[2],data.diagnosticAxes[3],data.touchCount};
                mapped=data.buttons;pressed=data.touchPressed!=0;x=data.leftX;y=data.leftY;
                move(i+4,axis(data.rightX)*dt*2.8f,axis(data.rightY)*dt*800.f);
                if(data.touchCount>0)touch=data.touchAverageX;
                // Pressed touches control transport; unpressed dragging keeps
                // card rotation / list scrolling. This logic is Rhine-owned.
                if(data.touchCount==1&&!pressed&&!touchPressed_[i]){
                    if(touchIds_[i]==data.singleTouchId)move(i+4,(data.singleTouchX-touchX_[i])*.0028f,(data.singleTouchY-touchY_[i])*.7f);
                    touchIds_[i]=data.singleTouchId;touchX_[i]=data.singleTouchX;touchY_[i]=data.singleTouchY;
                }else touchIds_[i]=-1;
            }else{
                names_[i+4].clear();touchIds_[i]=-1;touchWidth_[i]=0;
                if(result<0){padClose_(handles_[i]);handles_[i]=-1;}
            }
        }
        touchTransport_[i].sample(now,focused,connected,pressed,touch,[&](Action a){activeSlot_=int(i)+4;emit(a);});touchPressed_[i]=pressed;
        accept(i+4,mapped,x,y,now,focused,emit);
    }
}
int Controllers::vibration(size_t slot,bool on){
    if(slot<4){XINPUT_VIBRATION motors{WORD(on?xboxStrong:0),WORD(on?xboxWeak:0)};return int(XInputSetState(DWORD(slot),&motors));}
    if(slot>=8||handles_[slot-4]<0)return -1;return padVibrate_(handles_[slot-4],uint8_t(on?sonyStrong:0),uint8_t(on?sonyWeak:0));
}
bool Controllers::pulse(double now,double duration){
    bool accepted=false;vibrationResults_=json::array();duration=std::clamp(duration,.04,.3);
    for(size_t i=0;i<8;++i)if(!names_[i].empty()){int result=vibration(i,true);vibrationResults_.push_back({{"slot",i},{"name",names_[i]},{"strongMotor",i<4?unsigned(xboxStrong):unsigned(sonyStrong)},{"weakMotor",i<4?unsigned(xboxWeak):unsigned(sonyWeak)},{"durationMs",duration*1000},{"startResult",result}});if(result==0){vibrationUntil_[i]=now+duration;accepted=true;}}
    if(accepted)++vibrationPulses_;return accepted;
}
std::string Controllers::promptModel()const{int slot=activeSlot_;if(slot<0||names_[slot].empty()){slot=-1;for(size_t i=0;i<names_.size();++i)if(!names_[i].empty()){slot=int(i);break;}}return slot<0?"":slot<4?"xbox":names_[slot]=="DualSense"?"ps5":"ps4";}
json Controllers::status()const{json connected=json::array();for(size_t i=0;i<names_.size();++i)if(!names_[i].empty()){json item={{"slot",i},{"name",names_[i]}};if(i>=4){item["touchWidth"]=touchWidth_[i-4];auto raw=padRead_[i-4];item["lastRead"]={{"buttons",raw[0]},{"leftStick",json::array({raw[1],raw[2]})},{"rightStick",json::array({raw[3],raw[4]})},{"touches",raw[5]}};}connected.push_back(item);}return {{"vibration",{{"pulses",vibrationPulses_},{"stops",vibrationStops_},{"lastResults",vibrationResults_}}},{"padBridgeApi",RHINE_PAD_API_VERSION},{"padBridgeLoaded",bridge_!=nullptr},{"scePadInitialized",initialized_},{"connected",connected},{"promptModel",promptModel()},{"touchpad",{{"left","previous / hold rewind"},{"center","play / pause"},{"right","next / hold fast forward"},{"holdMs",500},{"seekSpeed",10},{"focusCycle","R3"}}}};}
}
