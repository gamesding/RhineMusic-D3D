#include "platform.hpp"
#include "controller-input.hpp"
#include <iostream>
#include <vector>
using namespace rhine;
int main(){try{
    json checks=json::array();
    auto test=[&](const char* label,const std::function<void(TouchpadTransport&,std::vector<Action>&)>& run){TouchpadTransport pad;std::vector<Action> actions;run(pad,actions);checks.push_back({{"name",label},{"pass",true}});};
    auto require=[](bool pass){if(!pass)throw std::runtime_error("Touchpad transport regression");};
    auto sample=[](TouchpadTransport& pad,std::vector<Action>& a,double t,bool down,std::optional<float> x=.1f,bool focus=true,bool connected=true){pad.sample(t,focus,connected,down,x,[&](Action action){a.push_back(action);});};
    test("left short click changes one track on release",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.2,true);require(a.empty());sample(p,a,.25,false);sample(p,a,.3,false);require(a==std::vector{Action::previousTrack});});
    test("right short click changes one track on release",[&](auto& p,auto& a){sample(p,a,0,true,.9f);sample(p,a,.2,false,.9f);require(a==std::vector{Action::next});});
    test("center press toggles once regardless of hold duration",[&](auto& p,auto& a){sample(p,a,0,true,.5f);sample(p,a,.6,true,.5f);sample(p,a,2,true,.5f);sample(p,a,2.1,false,.5f);require(a==std::vector{Action::play});});
    test("left hold seeks continuously and never skips on release",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.49,true);require(a.empty());sample(p,a,.5,true);sample(p,a,.61,true);sample(p,a,.72,true);sample(p,a,.8,false);require(a==std::vector{Action::seekBackward,Action::seekBackward,Action::seekBackward});});
    test("right hold seeks continuously and never skips on release",[&](auto& p,auto& a){sample(p,a,0,true,.9f);sample(p,a,.51,true,.9f);sample(p,a,.62,true,.9f);sample(p,a,.8,false,.9f);require(a==std::vector{Action::seekForward,Action::seekForward});});
    test("moving between regions during a click retains its original action",[&](auto& p,auto& a){sample(p,a,0,true,.1f);sample(p,a,.51,true,.5f);sample(p,a,.62,true,.9f);sample(p,a,.8,false,.9f);require(a==std::vector{Action::seekBackward,Action::seekBackward});});
    test("release without a held-state poll cannot become a short click",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.7,false,std::nullopt);require(a==std::vector{Action::seekBackward});});
    test("recent contact survives lifting the finger before clicking",[&](auto& p,auto& a){sample(p,a,0,false,.9f);sample(p,a,.1,true,std::nullopt);sample(p,a,.2,false,std::nullopt);require(a==std::vector{Action::next});});
    test("a stale contact defaults to center",[&](auto& p,auto& a){sample(p,a,0,false,.1f);sample(p,a,2,true,std::nullopt);sample(p,a,2.1,false,std::nullopt);require(a==std::vector{Action::play});});
    test("unpressed touch and drag emit no transport actions",[&](auto& p,auto& a){sample(p,a,0,false,.1f);sample(p,a,.7,false,.9f);require(a.empty());});
    test("focus loss cancels a short click and holding cannot reactivate it",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.1,true,.1f,false);sample(p,a,1,true);sample(p,a,1.1,false);require(a.empty());sample(p,a,1.2,true,.5f);require(a==std::vector{Action::play});});
    test("focus loss stops active rewind without skipping",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.51,true);sample(p,a,.62,true,.1f,false);sample(p,a,.73,true);sample(p,a,.8,false);require(a==std::vector{Action::seekBackward});});
    test("held input from an unfocused window never creates an edge",[&](auto& p,auto& a){sample(p,a,0,true,.5f,false);sample(p,a,.3,true,.5f);sample(p,a,.4,false,.5f);require(a.empty());});
    test("disconnect cancels the pending click; reconnect does not replay it",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,.2,false,std::nullopt,true,false);sample(p,a,.3,false);require(a.empty());sample(p,a,.4,true,.9f);sample(p,a,.5,false,.9f);require(a==std::vector{Action::next});});
    test("a stalled render frame produces bounded seek work",[&](auto& p,auto& a){sample(p,a,0,true);sample(p,a,20,true);sample(p,a,20.01,true);sample(p,a,20.02,false);require(a==std::vector{Action::seekBackward});});
    test("reconnecting a held touchpad waits for release and a new press",[&](auto& p,auto& a){sample(p,a,0,true,.1f);sample(p,a,.2,false,std::nullopt,true,false);sample(p,a,.3,true,.5f);sample(p,a,1,true,.5f);sample(p,a,1.1,false,.5f);require(a.empty());sample(p,a,1.2,true,.5f);require(a==std::vector{Action::play});});
    std::cout<<json({{"ok",true},{"checks",checks}}).dump(2)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
