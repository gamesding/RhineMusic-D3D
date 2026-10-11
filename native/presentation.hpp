#pragma once
#include "motion.hpp"
#include <functional>
#include <optional>
#include <string>
namespace rhine {
inline double cubicEase(double time,double x1,double y1,double x2,double y2){time=std::clamp(time,0.,1.);double low=0,high=1;for(int i=0;i<24;++i){double t=(low+high)*.5,u=1-t,x=3*u*u*t*x1+3*u*t*t*x2+t*t*t;if(x<time)low=t;else high=t;}double t=(low+high)*.5,u=1-t;return 3*u*u*t*y1+3*u*t*t*y2+t*t*t;}
class SurfaceMotion {
    double started_=0,from_=0,target_=0,duration_=0,fromOffset_=0,targetOffset_=0;bool browse_=false;std::function<void()> completion_;
public:
    double opacity=0,offset=0;
    explicit SurfaceMotion(bool browse=false):browse_(browse){}
    void move(bool show,double now,bool reduced,std::function<void()> completion={}){sample(now);from_=opacity;fromOffset_=opacity==0&&show&&!browse_?36:offset;target_=show?1:0;targetOffset_=show?0:browse_?0:52;started_=now;duration_=reduced?0:show?(browse_?.72:.36):(browse_?.14:.24);completion_=std::move(completion);if(reduced)sample(now);}
    void sample(double now){double t=duration_?std::clamp((now-started_)/duration_,0.,1.):1;double eased=target_==0?cubicEase(t,.4,0,1,1):browse_?cubicEase(t,.45,0,.25,1):cubicEase(t,.22,1,.36,1);opacity=std::lerp(from_,target_,eased);offset=std::lerp(fromOffset_,targetOffset_,eased);if(t>=1&&completion_){auto complete=std::move(completion_);completion_={};complete();}}
};
struct Selection {int index=0,direction=0;bool horizontal=false,viaArchive=false;};
struct PresentationPorts {
    std::function<bool()> ready,archiveReady;
    std::function<void()> enter,back,prepare,showMenu,showBrowse;
    std::function<void(const Selection&)> select,switchDetail;
    std::function<void(std::function<void()>)> hideMenu,hideBrowse;
    std::function<void(bool)> mode;
};
// Native translation of upstream music-presentation.ts. Surface lifetimes and
// scene motion have one owner, including input received while text is exiting.
class Presentation {
    PresentationPorts ports_;bool wantsDetail_=false,browseHidden_=false;unsigned revision_=0;
    void commit(bool detail){auto request=*pending;pending.reset();(detail?ports_.switchDetail:ports_.select)(request);if(detail)ports_.prepare();}
    void returnToArchive(){phase="returning";ports_.mode(false);ports_.back();}
    void beginOpen(){auto revision=++revision_;phase="opening";browseHidden_=false;ports_.mode(true);ports_.prepare();ports_.hideBrowse([this,revision]{if(revision==revision_)browseHidden_=true;});ports_.enter();}
    void beginExit(){if(phase=="hiding"||phase=="returning"||phase=="switch-hiding")return;auto revision=++revision_;phase="hiding";ports_.hideMenu([this,revision]{if(revision==revision_)returnToArchive();});}
    void beginSwitch(){auto revision=++revision_;phase="switch-hiding";if(!browseHidden_)ports_.hideBrowse([this,revision]{if(revision==revision_)browseHidden_=true;});ports_.hideMenu([this,revision]{if(revision!=revision_)return;if(!wantsDetail_||(pending&&pending->viaArchive)){returnToArchive();return;}phase="switching";if(pending)commit(true);});}
public:
    std::string phase="archive";std::optional<Selection> pending;
    explicit Presentation(PresentationPorts ports):ports_(std::move(ports)){}
    bool openingOrDetail()const{return wantsDetail_;}
    void open(){wantsDetail_=true;if(phase=="archive"){if(ports_.archiveReady())beginOpen();else{phase="selecting";ports_.hideBrowse([]{});}}}
    void back(){wantsDetail_=false;pending.reset();if(phase=="archive"||phase=="selecting")return;beginExit();}
    void select(Selection selection,bool openAfter){pending=selection;wantsDetail_=openAfter;bool detail=openAfter&&!selection.viaArchive;if(phase=="archive"){commit(false);if(openAfter){phase="selecting";ports_.hideBrowse([]{});}}else if(detail&&(phase=="detail"||phase=="opening"))beginSwitch();else if(detail&&(phase=="switch-hiding"||phase=="switching"))return;else if(phase!="selecting")beginExit();}
    void update(){if(phase=="switching"){if(pending){commit(true);return;}if(browseHidden_&&ports_.ready()){phase="detail";ports_.showMenu();}return;}if(phase=="opening"){if(browseHidden_&&ports_.ready()){phase="detail";ports_.showMenu();}return;}if(phase!="returning"&&phase!="selecting")return;if(!ports_.archiveReady())return;if(pending){commit(false);phase="selecting";return;}if(wantsDetail_)beginOpen();else{phase="archive";ports_.mode(false);ports_.showBrowse();}}
    void reset(){++revision_;pending.reset();wantsDetail_=browseHidden_=false;phase="archive";ports_.mode(false);}
};
}
