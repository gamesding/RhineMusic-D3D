#pragma once
#include "platform.hpp"
#include <cmath>
#include <map>
namespace rhine {
// Exact time-domain port of music-title-reels.ts: 460 ms, critical damping,
// sampled velocity on interruption, bounded visible faces and per-line keys.
struct TitleTrack {
    struct Sample {double value=0,velocity=0;};
    double start=0,target=0,velocity=0,started=0;bool moving=false;
    Sample sample(double now)const{
        const double t=std::clamp((now-started)/.46,0.,1.);if(!moving||t>=1)return {target,0};
        const double delta=start-target,advance=velocity*.46+10*delta,decay=std::exp(-10*t);
        return {target+(delta+advance*t)*decay,(advance-10*(delta+advance*t))*decay/.46};
    }
    void set(double value){start=target=value;velocity=0;moving=false;}
    void move(double value,double now){move(value,now,sample(now));}
    void move(double value,double now,Sample from){start=from.value;target=value;started=now;const double limit=std::max(std::abs(start-target),1.)*12/.46;velocity=std::clamp(from.velocity,-limit,limit);moving=std::abs(target-start)>=.001||std::abs(velocity)>=.001;}
    bool finished(double now)const{return !moving||now-started>=.46;}
};
struct TitleFace {
    std::wstring text;float font=40,height=56,width=0;int weight=700;float spacing=-.6f;bool translation=false;
    bool operator==(const TitleFace&)const=default;
};
struct TitleGlyph {std::string key;TitleFace face;float x=0,y=0;};
struct TitleSlot {
    TitleGlyph glyph;std::vector<TitleFace> faces;std::vector<float> offsets;TitleTrack roll,x,y,viewport;bool active=false;
    void write(std::vector<TitleFace> next){faces=std::move(next);offsets.clear();float offset=0;for(auto& face:faces){if(face.height==0)face.height=glyph.face.height;offsets.push_back(offset);offset+=face.height;}}
    void rollTo(TitleFace face,double now,bool animated){
        if(!animated){write({face});roll.set(0);return;}
        const auto sampled=roll.sample(now);const double position=std::max(0.,sampled.value);size_t first=0;
        while(first+1<offsets.size()&&offsets[first+1]<=position)++first;
        const double offset=offsets.empty()?0:std::max(0.,position-offsets[first]);
        std::vector<TitleFace> next{faces.empty()?TitleFace{L"",glyph.face.font,glyph.face.height}:faces[first]};
        const double height=viewport.sample(now).value;
        for(size_t i=first+1;i<faces.size();++i){if(offsets[i]>=position+height-.001)break;next.push_back(faces[i]);}
        if(next.back()!=face)next.push_back(face);write(std::move(next));const double target=offsets.back(),from=std::min(offset,target);
        roll.move(target,now,{from,std::clamp(sampled.velocity,0.,std::max(0.,(target-from)*10000/460))});
    }
    void settle(double now){if(roll.finished(now)&&faces.size()>1){auto last=faces.back();write({last});roll.set(0);}}
};
struct NativeTitle {
    std::wstring title;std::map<std::string,TitleSlot> slots;float font=40,height=0,width=0,budget=0;TitleTrack heightTrack;double updated=0;
    void apply(const std::vector<TitleGlyph>& glyphs,float nextHeight,double now,bool animated){
        const bool populated=!slots.empty();
        for(auto& [key,slot]:slots)slot.settle(now);
        std::vector<std::string> active;
        for(auto& glyph:glyphs){active.push_back(glyph.key);auto [where,created]=slots.try_emplace(glyph.key);auto& slot=where->second;
            if(created){slot.glyph=glyph;slot.write({TitleFace{L"",glyph.face.font,glyph.face.height}});slot.roll.set(0);slot.viewport.set(glyph.face.height);slot.x.set(glyph.x);slot.y.set(glyph.y);}
            const auto old=slot.glyph;const bool changed=!slot.active||old.face!=glyph.face;slot.glyph=glyph;
            if(old.x!=glyph.x||old.y!=glyph.y){if(animated){slot.x.move(glyph.x,now);slot.y.move(glyph.y,now);}else{slot.x.set(glyph.x);slot.y.set(glyph.y);}}
            if(old.face.height!=glyph.face.height){if(animated)slot.viewport.move(glyph.face.height,now);else slot.viewport.set(glyph.face.height);}
            if(changed)slot.rollTo(glyph.face,now,animated);else if(!animated)slot.roll.set(slot.roll.target);slot.active=true;
        }
        for(auto& [key,slot]:slots)if(slot.active&&std::find(active.begin(),active.end(),key)==active.end()){slot.active=false;auto blank=slot.glyph.face;blank.text.clear();slot.rollTo(blank,now,animated);}
        if(heightTrack.target!=nextHeight){if(animated&&populated)heightTrack.move(nextHeight,now);else heightTrack.set(nextHeight);}updated=now;
    }
    void collect(double now,bool reduced){
        size_t idle=0;for(auto& [key,slot]:slots){if(reduced){auto last=slot.faces.back();slot.write({last});slot.roll.set(0);slot.x.set(slot.x.target);slot.y.set(slot.y.target);slot.viewport.set(slot.viewport.target);}else slot.settle(now);if(!slot.active)++idle;}
        for(auto it=slots.begin();it!=slots.end()&&idle>64;){if(!it->second.active&&it->second.roll.finished(now)){it=slots.erase(it);--idle;}else++it;}
        if(reduced)heightTrack.set(heightTrack.target);height=float(heightTrack.sample(now).value);
    }
};
}
