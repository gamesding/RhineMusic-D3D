#pragma once
#include "platform.hpp"
#include "motion.hpp"
#include "layout.hpp"
#include <DirectXMath.h>
#include <map>
#include <optional>
namespace rhine {
using namespace DirectX;
struct Cell{int lane=0,row=12;bool operator==(const Cell&)const=default;};
struct alignas(16) Instance {XMFLOAT4X4 world{},previous{};XMFLOAT4 uv{},data{};};
struct SceneFrame {std::vector<Instance> instances;std::vector<int> albums;XMMATRIX view{},projection{};XMFLOAT3 camera{},aim{},selected{};float focus=140,span=7.33f,detail=0,night=0,rotation=0,nearPlane=5,farPlane=300;bool portrait=false,reset=false,viewer=false,reduced=false;};
class Scene {
    struct Outgoing{Cell cell;Spring lift;double rotation=0;int album=-1;int coverLayer=0;std::optional<double> heldY;};
    struct Pulse{Cell cell;double started;};
    std::vector<Outgoing> outgoing_;
    std::vector<Pulse> pulses_;
    std::map<std::pair<int,int>,XMFLOAT4X4> previous_;
    std::vector<std::vector<int>> files_;
    std::vector<bool> hiRes_,compactDiscs_;std::vector<unsigned> visibleDiscs_;std::map<std::string,double> discAngles_;std::string spinningDisc_;std::vector<std::string> albumIds_;
    Cell selectedCell_{};
    Spring column_{},rail_{},shoulder_{12},lane_{},lift_{.9},yawMotion_{},elevationMotion_{},rotation_{};
    std::array<Spring,3> camera_{},aim_{};Spring span_{7.33};
    Placement placement_;
    bool initialized_=false,detailRequested_=false,navigationLift_=false,boot_=true,reset_=true;
    double started_=0,lastInteraction_=0,lastFrameTime_=0,themeStarted_=0,themeFrom_=0,themeTarget_=0,night_=0,idle_=0,pulseGain_=1;
    double targetRotation_=0;std::optional<double> returnY_;
    int selected_=0,coverLayer_=0;bool archive_=false;
    double field(double row,double lane,double now,double shot,double introSettle)const;
    int albumAt(Cell cell)const;
public:
    void transport(const std::string& albumId,bool playing){spinningDisc_=playing?albumId:std::string();}
    json discStatus()const{json phases=json::object(),instances=json::object();for(const auto& [id,angle]:discAngles_)phases[id]=angle;for(size_t i=0;i<compactDiscs_.size();++i)if(compactDiscs_[i])instances[albumIds_[i]]=i<visibleDiscs_.size()?visibleDiscs_[i]:0;return {{"playingAlbum",spinningDisc_},{"angles",phases},{"visibleInstances",instances},{"turnsPerSecond",4},{"reducedMotion",lastReduced_}};}
    bool lastReduced_=false;
    void archive(bool value){archive_=value;}
    void resetRendering(){reset_=true;}
    void library(const json& library);
    void select(int album,double now,bool keepDetail=false,int direction=0,bool horizontal=false);
    void open(){if(selected_<0||albumIds_.empty())return;detailRequested_=true;returnY_.reset();}
    void back(){if(detailRequested_&&std::abs(rotation_.value)>.001)returnY_=-4.6+field(selectedCell_.row,selectedCell_.lane,lastFrameTime_,29.1,0)+lift_.value;detailRequested_=false;targetRotation_=0;}
    void skip(){boot_=false;reset_=true;}
    void theme(bool night,double now,bool reduced){themeFrom_=night_;themeTarget_=night?1:0;themeStarted_=now;if(reduced)night_=themeTarget_;}
    void rotate(double delta){targetRotation_=std::clamp(targetRotation_+delta,-.8,.8);}
    bool booting()const{return boot_;}
    bool detail()const{return detailRequested_;}
    bool ready()const{return placement_.settled()&&lift_.settled(detailRequested_?(archive_?4.05:3.47):(archive_?.4:.9))&&(!returnY_||std::abs(rotation_.value)<.001)&&column_.settled((selectedCell_.lane-2)*5.2)&&rail_.settled(-2.17-(selectedCell_.row-15.5)*.62);}
    bool canOpen()const{return !boot_&&column_.settled((selectedCell_.lane-2)*5.2)&&rail_.settled(-2.17-(selectedCell_.row-15.5)*.62)&&lift_.settled(archive_?.4:.9);}
    int selected()const{return selected_;}
    int selectedCoverLayer()const{return coverLayer_;}
    SceneFrame update(double now,double dt,int width,int height,bool reduced,XMFLOAT2 jitter={0,0},float dpi=1,bool coarse=false);
};
}
