#pragma once
#include "app.hpp"
#include <psapi.h>
namespace rhine {
class SelfTestScenario {
    struct Step {std::string name;double wait;std::function<bool(App&,const json&)> run;};
    HWND window_=nullptr;std::vector<Step> steps_;size_t step_=0;double entered_=-1,lastSample_=-1;std::string selectedId_,firstId_,lastTrack_;double radiusBefore_=0;int restoredSelection_=0;
public:
    json checks=json::array(),samples=json::array();bool finished=false;
    explicit SelfTestScenario(std::string name,HWND window):window_(window){
        if(name.empty()||name=="performance")return;if(name!="stress"&&name!="cd-stress"&&name!="covers-stress"&&name!="ui-parity"&&name!="controls-update"&&name!="controller-tracks"&&name!="cd-controls"&&name!="cd-media"&&name!="notifications"&&name!="notification-empty"&&name!="interaction-fixes"&&name!="search-input-fixes")throw std::runtime_error("Unknown native self-test scenario");
        auto action=[this](const char* label,double wait,std::string command,json args=json::object()){steps_.push_back({label,wait,[command,args](App& app,const json&){app.command(command,args);return true;}});};
        auto check=[this](const char* label,double wait,std::function<bool(const json&)> condition){steps_.push_back({label,wait,[condition](App&,const json& report){return condition(report);}});};
        auto resize=[this](const char* label,double wait,unsigned width,unsigned height){steps_.push_back({label,wait,[this,width,height](App&,const json&){return SetWindowPos(window_,nullptr,0,0,int(width),int(height),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=FALSE;}});};
        #include "parity-scenario.inc"
        #include "controls-scenario.inc"
        #include "interaction-scenario.inc"
        #include "search-input-scenario.inc"
        #include "controller-scenario.inc"
        #include "cd-scenario.inc"
        #include "cd-media-scenario.inc"
        #include "notification-scenario.inc"
        if(name=="covers-stress"){
            action("skip large fixture intro",0,"skip");
            check("720 isolated albums, no fabricated personal library",.1,[](auto& report){return report.value("albumCount",0)==720;});
            check("full atlas populated without decode cycling",.3,[](auto& report){return report.at("graphics").value("coverAtlasRows",0u)==27&&report.at("graphics").value("coverAtlasReadySlots",0u)>=400;});
            for(int i=0;i<120;++i)action("scroll large shelf through GPU cover eviction",.015,"navigate",{{"row",1}});
            check("atlas remains bounded after scrolling",1,[](auto& report){return report.at("graphics").value("coverAtlasRows",0u)==27&&report.at("graphics").value("coverAtlasBytes",uint64_t(0))==uint64_t(4096)*6912*4&&report.at("imageCache").value("decodedBytes",uint64_t(0))<=uint64_t(128)*1024*1024;});
            action("large library detail",.1,"open");
            check("selected full resolution cover and detail survive eviction",2,[](auto& report){return report.value("presentationPhase",std::string())=="detail"&&report.at("graphics").value("coverAtlasReadySlots",0u)>=400;});
            return;
        }
        if(name=="cd-stress"){
            action("skip intro",0,"skip");
            steps_.push_back({"play physical F drive audio CD",.1,[this](App& app,const json& report){const auto& album=report.at("selectedAlbum");if(!album.contains("cd")||album.at("cd").value("drive",std::string())!="F:")return false;selectedId_=album.value("id",std::string());auto track=std::find_if(album.at("tracks").begin(),album.at("tracks").end(),[](const auto& value){return value.value("duration",0.)>60;});if(track==album.at("tracks").end())return false;lastTrack_=track->value("id",std::string());app.command("play",{{"albumId",selectedId_},{"trackId",lastTrack_}});return true;}});
            check("CD decoded into native WASAPI output",.5,[](auto& report){const auto& player=report.at("player");return player.value("playing",false)&&player.value("currentTime",0.)>.3;});
            action("open CD detail while playing",.05,"open");
            resize("resize playing CD to portrait on DX11",1.5,900,1600);
            check("DX11 portrait resize preserves CD and detail",.5,[](auto& report){return report.at("graphics").value("width",0u)==900&&report.at("graphics").value("height",0u)==1600&&report.at("player").value("playing",false)&&report.value("presentationPhase",std::string())=="detail";});
            resize("return playing CD to landscape",.1,1280,720);
            action("switch active CD playback to DX12",1.5,"set",{{"key","graphicsApi"},{"value","dx12"}});
            action("DLSS during physical CD playback",.2,"set",{{"key","upscaler"},{"value","dlss"}});
            action("hardware spectral caustics during playback",.2,"set",{{"key","reflections"},{"value","rtx"}});
            check("CD continues during DLSS and DXR rebuild",1,[](auto& report){return report.at("player").value("playing",false)&&report.at("player").value("currentTime",0.)>2&&report.at("graphics").at("upscaler").value("active",false);});
            resize("resize active DLSS and DXR to ultrawide",.1,2100,900);
            check("DX12 ultrawide resize resets temporal history and preserves audio",.5,[](auto& report){return report.at("graphics").value("width",0u)==2100&&report.at("graphics").value("height",0u)==900&&report.at("graphics").at("upscaler").value("active",false)&&report.at("player").value("playing",false);});
            resize("resize DLSS and DXR to compact window",.1,800,600);
            check("compact DLSS window keeps valid targets",.5,[](auto& report){return report.at("graphics").value("width",0u)==800&&report.at("graphics").value("height",0u)==600&&report.at("player").value("playing",false);});
            resize("restore capture size during CD playback",.1,1280,720);
            action("open archives during CD playback",.1,"original",{{"enabled",true},{"view","detail"}});
            check("CD continues behind original archives",1.5,[](auto& report){return report.value("archive",false)&&report.at("player").value("playing",false);});
            action("return to CD detail",.05,"original",{{"enabled",false}});
            check("playing CD selection restored",.8,[this](auto& report){return !report.value("archive",true)&&report.at("selectedAlbum").value("id",std::string())==selectedId_&&report.at("player").at("currentTrack").value("id",std::string())==lastTrack_&&report.at("player").value("playing",false);});
            action("pause physical CD",.05,"toggle");
            check("native transport paused",.3,[](auto& report){return report.at("player").value("transport",std::string())=="paused";});
            action("seek physical CD to 20 seconds",.05,"seek",{{"position",20}});
            check("paused CD seek remains paused",.3,[](auto& report){return report.at("player").value("transport",std::string())=="paused"&&std::abs(report.at("player").value("currentTime",0.)-20)<.2;});
            action("resume physical CD",.05,"toggle");
            check("CD time advances after resume",.5,[](auto& report){return report.at("player").value("playing",false)&&report.at("player").value("currentTime",0.)>20.3;});
            action("restore DX11 with CD playing",.05,"set",{{"key","graphicsApi"},{"value","dx11"}});
            action("native resolution for playback screenshot",.2,"set",{{"key","upscaler"},{"value","native"}});
            check("CD and detail survive DX11 restoration",1,[](auto& report){return report.at("player").value("playing",false)&&report.value("presentationPhase",std::string())=="detail"&&report.at("graphics").value("api",std::string())=="Direct3D 11";});
            return;
        }
        action("skip intro",0,"skip");action("open interrupted detail",.05,"open");action("select forward during opening",.12,"navigate",{{"row",1}});action("select backward during opening",.08,"navigate",{{"row",-1}});action("cancel with back",.08,"back");action("reopen without jump",.1,"open");
        check("detail settled after rapid interruption",2,[](auto& report){return report.value("presentationPhase",std::string())=="detail"&&report.value("detailOpacity",0.f)>.99f;});
        steps_.push_back({"focus first song",.05,[](App& app,const json& report){if(report.at("ui").value("focus",std::string()).starts_with("track-"))return true;app.key(VK_TAB,false,false);return false;}});
        for(int i=0;i<17;++i)steps_.push_back({"controller / keyboard next song",.025,[](App& app,const json&){app.key(VK_DOWN,false,false);return true;}});
        check("virtualized track focus scrolls beyond first screen",.08,[](auto& report){return report.at("ui").value("focus",std::string()).starts_with("track-")&&report.at("ui").value("detailScroll",0.f)>100;});
        check("original tabs remain pinned above a scrolled track list",.08,[](auto& report){const auto& ui=report.at("ui");return ui.value("tabsPinned",false)&&std::abs(ui.value("tabsTop",0.f)-ui.value("detailPaneTop",-10.f))<.1f;});
        steps_.push_back({"click pinned intro tab through native pointer routing",.05,[](App& app,const json& report){auto p=report.at("ui").at("introTabCenterPixels");app.pointer(p[0].get<float>(),p[1].get<float>(),true,false,false,0);app.pointer(p[0].get<float>(),p[1].get<float>(),false,true,false,.1);return true;}});
        check("pinned tab switches to album introduction",.08,[](auto& report){return report.at("ui").value("detailTab",std::string())=="intro";});
        action("return to track tab",.05,"tab",{{"tab","tracks"}});
        action("detail-to-detail navigation",.05,"navigate",{{"lane",1}});
        check("detail navigation retains composition",2,[](auto& report){return report.value("presentationPhase",std::string())=="detail";});
        steps_.push_back({"locate same album / last track",.05,[this](App& app,const json& report){const auto& selected=report.at("selectedAlbum");selectedId_=selected.value("id",std::string());firstId_=report.at("albumIds")[0];const auto& tracks=selected.at("tracks");lastTrack_=tracks.back().value("id",std::string());app.command("locate",{{"albumId",selectedId_},{"trackId",lastTrack_}});return true;}});
        check("same-album locate does not replay opening",.12,[](auto& report){return report.value("presentationPhase",std::string())=="detail";});
        steps_.push_back({"cross-album location",.1,[this](App& app,const json&){app.command("locate",{{"albumId",firstId_}});return true;}});
        check("cross-album location returns and reopens",2,[this](auto& report){return report.at("selectedAlbum").value("id",std::string())==firstId_&&report.value("presentationPhase",std::string())=="detail";});
        action("switch theme",.05,"set",{{"key","theme"},{"value","night"}});action("retired model command",.1,"viewer-open");
        check("retired model stays closed and preserves album detail",.3,[](auto& report){return !report.at("viewer").value("enabled",true)&&!report.at("viewer").value("open",true)&&report.value("presentationPhase",std::string())=="detail";});
        steps_.push_back({"save music selection before archives",.05,[this](App& app,const json& report){restoredSelection_=report.value("selected",-1);app.command("original",{{"enabled",true},{"view","detail"}});return true;}});
        check("all original archives available",1.5,[](auto& report){return report.value("archive",false)&&report.value("albumCount",0)==40&&report.value("presentationPhase",std::string())=="detail";});
        action("archive 360 assembly",.05,"viewer-open");action("archive six groups exploded",.05,"viewer-explode");check("archive six-group viewer remains active",.5,[](auto& report){return report.value("archive",false)&&report.at("viewer").value("open",false);});action("close archive viewer",.05,"viewer-close");action("restore music scene",.05,"original",{{"enabled",false}});
        check("music scene and detail restored",.8,[this](auto& report){return !report.value("archive",true)&&report.value("albumCount",0)==3&&report.value("selected",-1)==restoredSelection_&&report.value("presentationPhase",std::string())=="detail";});
        action("switch to DX12",.05,"set",{{"key","graphicsApi"},{"value","dx12"}});action("select latest compatible FSR",.2,"set",{{"key","upscaler"},{"value","fsr"}});
        check("FSR runs on DX12",.8,[](auto& report){const auto& graphics=report.at("graphics");return graphics.value("api",std::string())=="Direct3D 12"&&graphics.at("upscaler").value("provider",std::string()).find("FSR")!=std::string::npos&&graphics.at("upscaler").value("active",false);});
        action("switch to NVIDIA DLSS",.05,"set",{{"key","upscaler"},{"value","dlss"}});check("DLSS runs on NVIDIA",.8,[](auto& report){return report.at("graphics").at("upscaler").value("provider",std::string()).find("DLSS")!=std::string::npos&&report.at("graphics").at("upscaler").value("active",false);});
        action("switch back to DX11",.05,"set",{{"key","graphicsApi"},{"value","dx11"}});action("select native rendering",.2,"set",{{"key","upscaler"},{"value","native"}});check("DX11 path restored",.8,[](auto& report){return report.at("graphics").value("api",std::string())=="Direct3D 11";});
        action("recover renderer while preserving audio and scene",.05,"graphics-recover");check("renderer recovery preserves detail state",.8,[](auto& report){return report.value("presentationPhase",std::string())=="detail"&&report.value("nativeAudioAvailable",false)&&report.value("sonyAvailable",false);});
    }
    bool scripted()const{return !steps_.empty();}
    void tick(App& app,double now){
        if(steps_.empty()&&now-lastSample_<1)return;
        if(entered_<0)entered_=now;const auto report=app.report();const auto& graphics=report.at("graphics");if(graphics.value("debugErrors",uint64_t(0))||graphics.value("debugDiscarded",uint64_t(0)))throw std::runtime_error("Graphics diagnostics failed during scenario");
        if(now-lastSample_>=1){lastSample_=now;PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));samples.push_back({{"time",now},{"privateBytes",memory.PrivateUsage},{"workingSetBytes",memory.WorkingSetSize},{"graphics",graphics}});}
        if(steps_.empty())return;if(step_>=steps_.size()){finished=true;return;}auto& step=steps_[step_];if(now-entered_<step.wait)return;
        if(step.run(app,report)){checks.push_back({{"name",step.name},{"time",now},{"pass",true},{"phase",report.value("presentationPhase",std::string())},{"api",graphics.value("api",std::string())}});++step_;entered_=now;}else if(now-entered_>step.wait+15)throw std::runtime_error("Native scenario timed out: "+step.name);
    }
};
}
