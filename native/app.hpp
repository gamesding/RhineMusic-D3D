#pragma once
#include "ui.hpp"
#include "service.hpp"
#include <optional>
#include "presentation.hpp"
#include "viewer.hpp"
namespace rhine {
class App {
    HWND window_;std::filesystem::path root_;std::string origin_;std::unique_ptr<Service> service_;std::unique_ptr<Renderer> renderer_;std::unique_ptr<Images> images_;std::unique_ptr<Ui> ui_;Controllers controllers_;Scene scene_;UiModel model_;GraphicsOptions graphics_;
    std::map<unsigned,std::function<void(const json&)>> replies_;std::map<unsigned,std::function<void(const std::string&)>> failures_;bool libraryReady_=false,fullscreen_=false,pendingOpen_=false,graphicsDirty_=false,forceGraphicsRebuild_=false;unsigned graphicsRecoveries_=0;double recoveryTime_=-100;RECT normalWindow_{};DWORD style_=0;double last_=0,controllerClock_=0,panelPoll_=0,graphicsStatusTime_=-1;int onlinePage_=1;
    enum class LocationPhase{none,returning,moving,opening};LocationPhase locationPhase_=LocationPhase::none;std::string pendingAlbum_,pendingTrack_;
    std::unique_ptr<Presentation> presentation_;SurfaceMotion detailMotion_,browseMotion_{true};float controllerScroll_=0;bool browseRevealed_=false;
    Viewer viewer_;
    std::set<std::string> knownCdIds_;std::string pendingCdSelection_;
    void observeCdLibrary(const json& library,const std::string& shelf);void updateCdSelection();
    bool isolatePhysicalInput_=false;uint64_t controllerActionSequence_=0;json controllerActions_=json::array();
    std::map<unsigned,XMFLOAT2> touches_;bool touchMultiple_=false;double touchDistance_=0;XMFLOAT2 touchCenter_{};
    json musicLibrary_=json::object();int musicSelection_=0;bool archivePending_=false;std::optional<Scene> musicScene_;std::optional<Presentation> musicPresentation_;std::optional<SurfaceMotion> musicDetailMotion_,musicBrowseMotion_;
    void archive(bool enabled,std::string view="browse");
    void recoverGraphics(const HresultError& error);
    void receive(const json& event);void navigate(const json& args);void locate(std::string album,std::string track);void panels(const std::string& panel);void preference(const json& args);void refreshGraphics();
    void request(std::string path,std::optional<json> body,std::function<void(const json&)> result={},std::function<void(const std::string&)> failure={});void notify(std::string message);void rpc(std::string operation,json args=json::object(),std::function<void(const json&)> result={},std::function<void(const std::string&)> failure={});void toggleFullscreen();void onlineSearch(bool more);bool selectedCd()const;void ejectCd();
public:
    App(HWND window,std::filesystem::path root,GraphicsOptions options,bool isolatePhysicalInput=false);
    ~App();
    void tick(double now,bool renderFrame=true);
    void command(std::string action,json args=json::object());
    void mediaChange(DWORD unitMask,bool inserted);
    void error(std::string message){notify(std::move(message));}
    void resize(unsigned width,unsigned height);
    void pointer(float x,float y,bool down,bool up,bool cancel,double now){if(down)model_.controllerActive=false;ui_->pointer(x,y,down,up,cancel,now);}
    bool touch(unsigned id,float x,float y,bool down,bool up,bool cancel,double now);
    void cancelTouches(){touches_.clear();touchMultiple_=false;touchDistance_=0;ui_->pointer(0,0,false,false,true,model_.now);}
    void wheel(float pixels){ui_->wheel(pixels);}
    void key(UINT code,bool shift,bool ctrl);
    void controller(Action action,bool physical=false);
    void fieldChanged(HWND handle){ui_->fieldChanged(handle);}
    void fieldCommit(HWND handle){ui_->fieldCommit(handle);}
    bool editing()const{return ui_->editing();}
    HBRUSH editorColor(HDC context)const{return ui_->editorColor(context);}
    bool loaded()const{return libraryReady_;}
    void capture(const std::filesystem::path& image){renderer_->capture(image);tick(model_.now);}
    json report()const;
};
}
