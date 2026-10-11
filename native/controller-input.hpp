#pragma once
#include <algorithm>
#include <functional>
#include <optional>
namespace rhine {
enum class Action { up,down,left,right,confirm,back,play,previous,next,settings,search,library,tab,stop,seekBackward,seekForward,previousTrack,faceLeft };

// Shared by the real Pad SDK route and deterministic input regression. A
// click captures its region until release; moving the finger cannot switch
// from seeking to pause. Losing focus or disconnecting cancels that click.
class TouchpadTransport {
    bool pressed_=false,armed_=false,held_=false,needsNeutral_=false;
    int region_=0;
    double began_=0,nextSeek_=0,lastTouch_=-100;
    float touchX_=.5f;
public:
    static constexpr double holdSeconds=.5,seekInterval=.1;
    void reset(){pressed_=armed_=held_=false;lastTouch_=-100;needsNeutral_=true;}
    void sample(double now,bool focused,bool connected,bool pressed,std::optional<float> x,const std::function<void(Action)>& emit){
        if(!connected){reset();return;}
        if(needsNeutral_){pressed_=pressed;if(pressed)return;needsNeutral_=false;}
        if(x){touchX_=std::clamp(*x,0.f,1.f);lastTouch_=now;}
        if(pressed&&!pressed_){
            float point=now-lastTouch_<=.15?touchX_:.5f;
            region_=point<1.f/3?-1:point>=2.f/3?1:0;
            began_=now;nextSeek_=now+holdSeconds;held_=false;armed_=focused;
            if(armed_&&region_==0){emit(Action::play);armed_=false;}
        }
        if(!focused)armed_=false;
        if(pressed&&armed_&&region_!=0&&now>=nextSeek_){
            held_=true;nextSeek_=now+seekInterval;
            emit(region_<0?Action::seekBackward:Action::seekForward);
        }
        if(!pressed&&pressed_&&armed_&&region_!=0&&!held_){
            emit(now-began_>=holdSeconds?(region_<0?Action::seekBackward:Action::seekForward):(region_<0?Action::previousTrack:Action::next));
        }
        if(!pressed)armed_=false;
        pressed_=pressed;
    }
};
}
