#pragma once
#include "platform.hpp"
#include "controller-input.hpp"
#include "pad-bridge-api.h"
#include <array>
#include <functional>
namespace rhine {
class Controllers {
    std::array<int,4> handles_{-1,-1,-1,-1};
    std::array<unsigned,8> buttons_{};
    std::array<int,8> directions_{};
    std::array<double,8> nextRepeat_{};
    HMODULE bridge_=nullptr;
    decltype(&rhine_pad_initialize) padInitialize_=nullptr;
    decltype(&rhine_pad_shutdown) padShutdown_=nullptr;
    decltype(&rhine_pad_open) padOpen_=nullptr;
    decltype(&rhine_pad_close) padClose_=nullptr;
    decltype(&rhine_pad_read) padReadState_=nullptr;
    decltype(&rhine_pad_vibrate) padVibrate_=nullptr;
    bool initialized_=false;
    double nextOpen_=0;
    std::array<std::string,8> names_{};
    double lastPoll_=0;std::array<int,4> touchIds_{-1,-1,-1,-1};std::array<float,4> touchX_{},touchY_{};
    std::array<TouchpadTransport,4> touchTransport_{};
    std::array<unsigned,4> touchWidth_{};
    std::array<bool,4> touchPressed_{};
    int activeSlot_=-1;
    std::array<std::array<unsigned,6>,4> padRead_{};
    std::array<double,8> vibrationUntil_{};unsigned vibrationPulses_=0,vibrationStops_=0;json vibrationResults_=json::array();
    int vibration(size_t slot,bool on);
    void accept(size_t slot,unsigned buttons,float x,float y,double now,bool focused,const std::function<void(Action)>& emit);
public:
    Controllers();
    ~Controllers();
    Controllers(const Controllers&)=delete;
    Controllers& operator=(const Controllers&)=delete;
    void poll(double now,bool focused,const std::function<void(Action)>& emit,const std::function<void(float,float)>& analog={});
    json status()const;
    std::string promptModel()const;
    bool pulse(double now,double duration=.16);
    void feedback(double now,bool focused);
};
}
