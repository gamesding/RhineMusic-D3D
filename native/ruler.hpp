#pragma once
#include "motion.hpp"
#include <string>
#include <vector>
#include <set>
namespace rhine {
class NativeRuler {
public:
    struct Slot {int ordinal=0;double x=0;Spring offset{},presence{},height{10},opacity{.28};double revealAt=0,rippleAt=-1e30;bool visible=false;};
    std::array<Slot,16> slots{};Spring scroll{},width{};int selectedOrdinal=0;double tickWidth=3,gap=8,restHeight=10,fullHeight=30,hoverHeight=22;
private:
    std::vector<int> items_;std::string identity_;bool populated_=false;int selected_=-1;double scrollTarget_=0;
    double step()const{return tickWidth+gap;}bool overflowing()const{return items_.size()>12;}
    bool valid(int ordinal)const{return !items_.empty()&&(overflowing()||ordinal>=0&&ordinal<int(items_.size()));}
    void regroup(double now,bool instant){
        scrollTarget_=overflowing()?selectedOrdinal-5:0;scroll={scrollTarget_,0};const int start=overflowing()?int(std::floor(scroll.value))-2:0;
        std::array<int,16> ordinals{};for(int i=0;i<16;++i)ordinals[i]=start+i;std::stable_sort(ordinals.begin(),ordinals.end(),[&](int a,int b){auto visible=[&](int ordinal){return valid(ordinal)&&ordinal-scroll.value>=0&&ordinal-scroll.value<12;};return visible(a)>visible(b);});
        std::set<int> available;for(int i=0;i<16;++i)available.insert(i);
        for(int ordinal:ordinals){const double x=(ordinal-scroll.value)*step();int index=*std::min_element(available.begin(),available.end(),[&](int a,int b){return std::abs(slots[a].x-x)<std::abs(slots[b].x-x);});available.erase(index);auto& slot=slots[index];const bool shown=slot.presence.value>.025&&slot.x+tickWidth>.5&&slot.x<width.value-.5;slot.ordinal=ordinal;slot.offset.value=instant||!shown?0:slot.x-x;if(instant||!shown)slot.offset.velocity=0;
            if(!shown){slot.presence={};slot.height={restHeight,0};slot.opacity={.28,0};}const double delay=instant?0:std::clamp(ordinal-scroll.value,0.,11.)*.024;const bool changes=valid(ordinal)?slot.presence.value<.025:slot.presence.value>.025;slot.revealAt=changes?now+delay:now;slot.rippleAt=instant?-1e30:now+delay;}
    }
    void recycle(){if(!overflowing())return;const int start=int(std::floor(scroll.value))-2,end=start+16;std::set<int> present;std::vector<Slot*> free;for(auto& slot:slots){if(slot.ordinal>=start&&slot.ordinal<end)present.insert(slot.ordinal);else{const double x=(slot.ordinal-scroll.value)*step()+slot.offset.value;if(slot.presence.value<=.025||x+tickWidth<=.5||x>=width.value-.5)free.push_back(&slot);}}size_t index=0;for(int ordinal=start;ordinal<end&&index<free.size();++ordinal)if(!present.contains(ordinal)){auto& slot=*free[index++];slot.ordinal=ordinal;slot.offset={};slot.presence={1,0};slot.height={ordinal==selectedOrdinal?fullHeight:restHeight,0};slot.opacity={ordinal==selectedOrdinal?1.:.28,0};slot.revealAt=0;slot.rippleAt=-1e30;}}
public:
    NativeRuler(){for(int i=0;i<16;++i){slots[i].ordinal=i;slots[i].x=i*step();}}
    void update(std::vector<int> items,int selected,const std::string& identity,int direction,double now,double dt,bool reduced,bool compact,int hovered,int focused){
        const double oldStep=step();tickWidth=compact?2:3;gap=compact?6:8;restHeight=compact?8:10;fullHeight=compact?21:30;hoverHeight=compact?16:22;
        if(step()!=oldStep){const double ratio=step()/oldStep;width.value*=ratio;width.velocity*=ratio;for(auto& slot:slots){slot.x*=ratio;slot.offset.value*=ratio;slot.offset.velocity*=ratio;}}
        const bool first=!populated_,group=identity!=identity_,changed=selected_!=selected;auto found=std::find(items.begin(),items.end(),selected);int row=found==items.end()?0:int(found-items.begin());items_=std::move(items);identity_=identity;selected_=selected;
        if(group||first){selectedOrdinal=row;regroup(now,first||reduced);}else if(changed||direction){if(overflowing()){if(direction){int candidate=selectedOrdinal+direction;if(wrap(candidate,int(items_.size()))==row)selectedOrdinal=candidate;else selectedOrdinal+=direction>0?wrap(row-selectedOrdinal,int(items_.size())):-wrap(selectedOrdinal-row,int(items_.size()));}else selectedOrdinal=nearest(row,scroll.value+5,int(items_.size()));scrollTarget_=selectedOrdinal-5;}else selectedOrdinal=row;}
        const bool instant=first||reduced;populated_=populated_||!items_.empty();scroll.update(scrollTarget_,18,dt,instant);width.update(std::max(0.,std::min(size_t(12),items_.size())*step()-gap),18,dt,instant);recycle();
        for(auto& slot:slots){const bool exists=valid(slot.ordinal),active=exists&&slot.ordinal==selectedOrdinal;if(instant){slot.revealAt=now;slot.rippleAt=-1e30;}const bool shown=now>=slot.revealAt;slot.presence.update(exists?double(shown):double(!shown),18,dt,instant);slot.offset.update(0,18,dt,instant);slot.x=(slot.ordinal-scroll.value)*step()+slot.offset.value;slot.visible=exists&&shown&&slot.presence.value>.025&&slot.x+tickWidth>.5&&slot.x<width.value-.5;
            const bool hovering=slot.visible&&(slot.ordinal==hovered||slot.ordinal==focused);const double progress=(now-slot.rippleAt)/.56,contraction=!instant&&exists&&progress>=0&&progress<1?.4*std::sin(3.141592653589793*progress):0;slot.height.update(active?fullHeight:hovering?hoverHeight:restHeight*(1-contraction),18,dt,instant);slot.opacity.update(active||hovering?1:.28-contraction*.2,18,dt,instant);}
    }
    int album(int ordinal)const{return valid(ordinal)?items_[wrap(ordinal,int(items_.size()))]:-1;}
    bool overflows()const{return overflowing();}
};
}
