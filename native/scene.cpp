#include "scene.hpp"
namespace rhine {
void Scene::library(const json& library){
    auto oldIds=albumIds_;files_.clear();hiRes_.clear();compactDiscs_.clear();albumIds_.clear();for(auto& column:library.value("columnFiles",json::array()))files_.push_back(column.get<std::vector<int>>());for(auto& album:library.value("albums",json::array())){hiRes_.push_back(album.value("hiRes",false));compactDiscs_.push_back(album.contains("cd"));if(album.contains("cd"))discAngles_.try_emplace(album.value("id",std::string()),0.);albumIds_.push_back(album.value("id",""));}for(auto& old:outgoing_){if(old.album>=0&&old.album<int(oldIds.size())){auto found=std::find(albumIds_.begin(),albumIds_.end(),oldIds[old.album]);old.album=found==albumIds_.end()?-1:int(found-albumIds_.begin());}}if(selected_>=0&&selected_<int(oldIds.size())){auto found=std::find(albumIds_.begin(),albumIds_.end(),oldIds[selected_]);selected_=found==albumIds_.end()?(albumIds_.empty()?-1:0):int(found-albumIds_.begin());}
    if(!initialized_||(selected_<0&&!albumIds_.empty())){for(size_t lane=0;lane<files_.size();++lane)if(!files_[lane].empty()){selected_=files_[lane][0];selectedCell_={int(lane),12};break;}column_={(selectedCell_.lane-2)*5.2,0};rail_={-2.17-(selectedCell_.row-15.5)*.62,0};lane_={double(selectedCell_.lane),0};shoulder_={double(selectedCell_.row),0};reset_=true;}
    if(albumIds_.empty())selected_=-1;if(!initialized_)reset_=true;
}
int Scene::albumAt(Cell cell)const{if(files_.empty())return -1;auto& files=files_[wrap(cell.lane,int(files_.size()))];return files.empty()?-1:files[wrap(cell.row-12,int(files.size()))];}
void Scene::select(int album,double now,bool keepDetail,int direction,bool horizontal){
    const bool disc=album>=0&&album<int(compactDiscs_.size())&&compactDiscs_[album];
    Cell next=selectedCell_;bool found=false;for(size_t lane=0;lane<files_.size();++lane){auto where=std::find(files_[lane].begin(),files_[lane].end(),album);if(where==files_[lane].end())continue;const int row=12+int(where-files_[lane].begin());next={nearest(int(lane),selectedCell_.lane,int(files_.size())),disc?row:nearest(row,selectedCell_.row,int(files_[lane].size()))};found=true;break;}
    if(!found)return;
    if(disc&&album==selected_){selectedCell_=next;return;} // A single drive has no duplicate card to cycle to.
    if(direction){if(horizontal)next.lane=selectedCell_.lane+direction;else if(!disc)next={selectedCell_.lane,selectedCell_.row+direction};}
    if(next==selectedCell_){selected_=album;return;}
    std::optional<double> held;if(std::abs(rotation_.value)>.001)held=-4.6+field(selectedCell_.row,selectedCell_.lane,now,29.1,0)+lift_.value;outgoing_.push_back({selectedCell_,lift_,rotation_.value,selected_,coverLayer_,held});returnY_.reset();if(outgoing_.size()>6)outgoing_.erase(outgoing_.begin());
    auto returning=std::find_if(outgoing_.begin(),outgoing_.end(),[next](auto& old){return old.cell==next;});
    lift_={0,0};rotation_={0,0};coverLayer_=(coverLayer_+1)%8;
    if(returning!=outgoing_.end()){lift_=returning->lift;rotation_.value=returning->rotation;coverLayer_=returning->coverLayer;outgoing_.erase(returning);}
    selected_=album;selectedCell_=next;lastInteraction_=now;targetRotation_=0;
    if(keepDetail&&detailRequested_)navigationLift_=true;
    if(!keepDetail||!detailRequested_)pulses_.push_back({next,now});
}
double Scene::field(double row,double lane,double now,double shot,double introSettle)const{
    if(boot_){double opening=cinematicField(row-selectedCell_.row+12,lane-selectedCell_.lane+2,shot);double resting=settlingWave(row-selectedCell_.row)*columnStrength(lane,selectedCell_.lane);return opening+(resting-opening)*introSettle;}
    double height=idleWave(row,lane,now)*idle_,ripple=0;for(auto& pulse:pulses_)ripple+=musicSelectionWave(std::hypot(row-pulse.cell.row,(lane-pulse.cell.lane)*2.2),now-pulse.started);height+=std::clamp(ripple,-.24,.24)*pulseGain_;return height+settlingWave(row-shoulder_.value)*columnStrength(lane,lane_.value);
}
SceneFrame Scene::update(double now,double dt,int width,int height,bool reduced,XMFLOAT2 jitter,float dpi,bool coarse){
    lastReduced_=reduced;lastFrameTime_=now;dt=std::clamp(dt,0.,.05);width=std::max(1,width);height=std::max(1,height);double aspect=double(width)/height;const double previewLift=archive_?.4:.9,inspectionLift=archive_?4.05:3.47;auto layout=viewportLayout(width,height,dpi,coarse);bool portrait=layout.portrait;if(!reduced&&!spinningDisc_.empty()&&discAngles_.contains(spinningDisc_))discAngles_[spinningDisc_]=std::fmod(discAngles_[spinningDisc_]+dt*8*XM_PI,2*XM_PI);
    if(!initialized_){started_=now;lastInteraction_=now;initialized_=true;}
    double elapsed=now-started_;if(reduced||elapsed>=5.2)boot_=false;
    double shot=boot_?21.92+std::min(5.2,elapsed):29.1,introSettle=boot_?smooth((shot-25.3)/1.42):0;
    if(reduced||elapsed>=5.2)boot_=false;
    night_=themeFrom_+(themeTarget_-themeFrom_)*(reduced?1:themeCurve((now-themeStarted_)/.65));
    if(reduced)rotation_={detailRequested_?targetRotation_:0,0};else if(detailRequested_){rotation_.value+=(targetRotation_-rotation_.value)*(1-std::exp(-2.8*dt));rotation_.velocity=(targetRotation_-rotation_.value)*2.8;}else{rotation_.value*=std::exp(-7*dt);if(std::abs(rotation_.value)<=.001)rotation_={};}
    bool turningBack=!detailRequested_&&std::abs(rotation_.value)>.001;
    double progress=placement_.update(detailRequested_||turningBack?1:0,dt,reduced);
    double switchRate=navigationLift_&&detailRequested_?9:3.7;
    column_.update((selectedCell_.lane-2)*5.2,boot_?35:switchRate,dt,reduced);
    rail_.update(-2.17-(selectedCell_.row-15.5)*.62,boot_?35:switchRate,dt,reduced);
    shoulder_.update(selectedCell_.row,navigationLift_?9:4.2,dt,reduced);lane_.update(selectedCell_.lane,navigationLift_?9:4.2,dt,reduced);
    if(boot_){column_.value=(selectedCell_.lane-2)*5.2;rail_.value=-2.17-(selectedCell_.row-15.5)*.62;lift_.value=extraction(shot)+(previewLift-extraction(shot))*introSettle;}
    else if(navigationLift_)lift_.update(detailRequested_?inspectionLift:previewLift,detailRequested_?9:4.2,dt,reduced);
    else {lift_.value=previewLift+(inspectionLift-previewLift)*progress;lift_.velocity=(inspectionLift-previewLift)*placement_.velocity;}
    if(returnY_&&turningBack){lift_.value=*returnY_+4.6-field(selectedCell_.row,selectedCell_.lane,now,shot,introSettle);lift_.velocity=0;}else returnY_.reset();if(progress==0&&placement_.settled())navigationLift_=false;
    bool idle=!boot_&&!reduced&&!detailRequested_&&progress<.01&&outgoing_.empty()&&now-lastInteraction_>2.5;
    idle_+=(double(idle)-idle_)*(1-std::exp(-dt*(idle?.8:4)));pulseGain_+=(double(!detailRequested_&&!turningBack)-pulseGain_)*(1-std::exp(-dt*8));
    pulses_.erase(std::remove_if(pulses_.begin(),pulses_.end(),[now](auto& pulse){return now-pulse.started>3.2;}),pulses_.end());
    SceneFrame frame;frame.reduced=reduced;frame.detail=float(progress);frame.night=float(night_);frame.portrait=portrait;frame.reset=std::exchange(reset_,false);frame.rotation=float(rotation_.value);
    double trackX=column_.value,entrance=std::clamp((shot-21.92)/.75,0.,1.),entryZ=boot_?-23*(1-entrance)*(1-entrance):0;
    std::map<std::pair<int,int>,XMFLOAT4X4> nextPrevious;
    auto make=[&](Cell cell,double lift,double angle,int album,int tile,int layer){
        double slope=field(cell.row+.5,cell.lane,now,shot,introSettle)-field(cell.row-.5,cell.lane,now,shot,introSettle);
        double x=(cell.lane-2)*5.2-trackX,y=-4.6+field(cell.row,cell.lane,now,shot,introSettle)+lift,z=(cell.row-15.5)*.62+entryZ+rail_.value;
        XMMATRIX world=XMMatrixRotationRollPitchYaw(float(slope*.024*(1-progress)*(1-smooth(lift/.4))),float(angle),0)*XMMatrixTranslation(float(x),float(y),float(z));
        Instance item;XMStoreFloat4x4(&item.world,world);auto key=std::pair{cell.lane,cell.row};item.previous=previous_.contains(key)?previous_.at(key):item.world;
        item.uv={float(tile%16)/16.f,float(tile/16)/27.f,1.f/16.f,1.f/27.f};item.data={album>=0&&album<int(hiRes_.size())&&hiRes_[album]?1.f:0.f,float(lift>=previewLift?progress:0),float(layer),0};
        if(album>=0&&album<int(compactDiscs_.size())&&compactDiscs_[album])item.data.x=-1.f-float(discAngles_.at(albumIds_[album]));
        frame.instances.push_back(item);frame.albums.push_back(album);nextPrevious[key]=item.world;return XMFLOAT3(float(x),float(y),float(z));
    };
    // Ordinary albums keep the original 9 x 48 cyclic shelf. Optical media
    // use one finite row per populated drive, including virtual CD drives.
    std::map<int,Cell> discCells;
    for(size_t lane=0;lane<files_.size();++lane)for(size_t row=0;row<files_[lane].size();++row){int album=files_[lane][row];if(album>=0&&album<int(compactDiscs_.size())&&compactDiscs_[album])discCells.try_emplace(album,Cell{nearest(int(lane),column_.value/5.2+2,int(files_.size())),12+int(row)});}
    std::map<int,size_t> movingDiscs;
    for(size_t i=0;i<outgoing_.size();++i){int album=outgoing_[i].album;if(album>=0&&album<int(compactDiscs_.size())&&compactDiscs_[album]&&album!=selected_)movingDiscs[album]=i;}
    auto hideLast=[&]{auto& instance=frame.instances.back();XMStoreFloat4x4(&instance.world,XMMatrixScaling(0,0,0)*XMLoadFloat4x4(&instance.world));};
    const int lanes[]={0,1,2,3,4,-2,-1,5,6};
    const int rows=archive_?32:48;for(int i=0;i<432;++i){const int active=9*rows;Cell cell{nearest(lanes[(i%active)/rows],column_.value/5.2+2,9),nearest(i%rows-8,(-rail_.value-2.17)/.62+15.5,rows)};int album=albumAt(cell);bool hidden=i>=active||albumIds_.empty()||cell==selectedCell_||std::any_of(outgoing_.begin(),outgoing_.end(),[cell](auto& old){return old.cell==cell;});if(discCells.contains(album))hidden=hidden||cell!=discCells.at(album)||album==selected_||movingDiscs.contains(album);make(cell,0,0,album,i,-1);if(hidden)hideLast();}
    for(size_t i=0;i<outgoing_.size();++i){auto& old=outgoing_[i];old.rotation=reduced?0:old.rotation*std::exp(-dt*7);if(std::abs(old.rotation)<=.001)old.rotation=0;if(reduced&&navigationLift_){old.heldY.reset();old.lift={};}else if(old.heldY){old.lift.value=*old.heldY+4.6-field(old.cell.row,old.cell.lane,now,shot,introSettle);old.lift.velocity=0;if(old.rotation==0)old.heldY.reset();}else old.lift.update(0,navigationLift_?9:4.5,dt,reduced);make(old.cell,old.lift.value,old.rotation,old.album,0,old.coverLayer);if(old.album<0||(discCells.contains(old.album)&&(!movingDiscs.contains(old.album)||movingDiscs.at(old.album)!=i)))hideLast();}
    outgoing_.erase(std::remove_if(outgoing_.begin(),outgoing_.end(),[](auto& old){return old.lift.value<.0001&&old.rotation==0;}),outgoing_.end());
    frame.selected=make(selectedCell_,lift_.value,rotation_.value,selected_,0,coverLayer_);if(albumIds_.empty()){XMStoreFloat4x4(&frame.instances.back().world,XMMatrixScaling(0,0,0));}previous_=std::move(nextPrevious);
    visibleDiscs_.assign(albumIds_.size(),0);
    for(size_t i=0;i<frame.instances.size();++i){int album=frame.albums[i];const auto& world=frame.instances[i].world;if(album>=0&&album<int(compactDiscs_.size())&&compactDiscs_[album]&&world._11*world._11+world._12*world._12+world._13*world._13>.5f)++visibleDiscs_[album];}
    double orbit=smooth((shot-22.6)/1.6),settle=smooth((shot-24.25)/2.25);
    yawMotion_.update(std::clamp(column_.velocity/5.2/2.5,-1.,1.)*.018*(1-progress),6,dt,reduced);elevationMotion_.update(std::clamp(rail_.velocity/.62/3.,-1.,1.)*.006*(1-progress),6,dt,reduced);
    double yaw=(89-22*orbit-8*settle)*XM_PI/180+yawMotion_.value,elevation=(3+40*smooth((shot-21.96)/.22)-8*orbit-16*settle+6*(archive_?0:boot_?introSettle:1))*XM_PI/180+elevationMotion_.value;
    yaw=yaw+((archive_?16.567:8)*XM_PI/180-yaw)*progress;elevation=elevation+((archive_?13.77:20)*XM_PI/180-elevation)*progress;
    double span=(10.8+(10.3-10.8)*orbit)+(7.33-(10.8+(10.3-10.8)*orbit))*settle,distance=(28+7*orbit)+(140-(28+7*orbit))*settle;distance+=(72-distance)*progress;
    double baseSpan=span+(5.9-span)*progress,portraitDetail=std::max(6.3/aspect,3.7*layout.height/std::max(100.,.54*layout.height-156));span=portrait?std::max(baseSpan,8.4/aspect+(portraitDetail-8.4/aspect)*progress):std::max(baseSpan,baseSpan*(16./9)/aspect);
    XMVECTOR direction=XMVectorSet(float(-std::sin(yaw)*std::cos(elevation)),float(std::sin(elevation)),float(std::cos(yaw)*std::cos(elevation)),0),right=XMVector3Normalize(XMVector3Cross(XMVectorSet(0,1,0,0),direction)),up=XMVector3Normalize(XMVector3Cross(direction,right));
    XMVECTOR aim=XMVectorSet(-1.091f,float((-2.55+.4*orbit)+(-.045-(-2.55+.4*orbit))*settle),float(2.48+(.481-2.48)*settle),0);
    XMVECTOR preview=aim;
    if(portrait)preview=XMVectorSet(0,float(-4.6+settlingWave(0)+.4+1.85),-2.17f,0)+up*float((.36-.5)*span);
    if(!portrait&&!archive_){double wide=smooth((aspect-16./9)/(21./9-16./9));preview+=right*float(.035*wide*span*aspect)+up*float((.08+.02*wide)*span);}
    if(boot_){double pan=smooth((shot-25.4)/.95);aim+=right*float(-2.05*(1-pan)*smooth((shot-24.2)/.8));if(shot>=25.05&&shot<=27.3){double cornerPan=smooth((shot-25.4)/1.05),pixels=1080/((10.8+(10.3-10.8)*orbit)+(7.33-(10.8+(10.3-10.8)*orbit))*settle);XMVECTOR corner=XMLoadFloat3(&frame.selected)+XMVectorSet(-2.225f,3.525f,0,0);corner-=right*float((840+(518-840)*cornerPan-960)/pixels)+up*float((540-(340+(288-340)*cornerPan))/pixels);aim=XMVectorLerp(aim,corner,float(smooth((shot-25.05)/.35)));}aim=XMVectorLerp(aim,preview,float(introSettle));}else aim=preview;
    XMVECTOR detailAim=XMVectorSet(0,float(-4.6+settlingWave(0)+inspectionLift+1.85),-2.17f,0);double detailX=portrait?.5:archive_?(layout.compact?.27:550./1920):.25,detailY=portrait?.27+34./layout.height:(layout.compact?.49:560./1080);
    detailAim+=right*float((.5-detailX)*span*aspect)+up*float((detailY-.5)*span);aim=XMVectorLerp(aim,detailAim,float(progress));
    XMVECTOR position=aim+direction*float(distance);XMFLOAT3 desiredPosition,desiredAim;XMStoreFloat3(&desiredPosition,position);XMStoreFloat3(&desiredAim,aim);
    float* pos=&desiredPosition.x;float* target=&desiredAim.x;for(int axis=0;axis<3;++axis){if(boot_||frame.reset){camera_[axis]={pos[axis],0};aim_[axis]={target[axis],0};}else{camera_[axis].update(pos[axis],9,dt,reduced);aim_[axis].update(target[axis],9,dt,reduced);}}
    if(boot_||frame.reset)span_={span,0};else span_.update(span,9,dt,reduced);
    frame.camera={float(camera_[0].value),float(camera_[1].value),float(camera_[2].value)};frame.aim={float(aim_[0].value),float(aim_[1].value),float(aim_[2].value)};
    float actualDistance=XMVectorGetX(XMVector3Length(XMLoadFloat3(&frame.camera)-XMLoadFloat3(&frame.aim)));frame.span=float(span_.value);frame.focus=actualDistance;
    frame.view=XMMatrixLookAtRH(XMLoadFloat3(&frame.camera),XMLoadFloat3(&frame.aim),XMVectorSet(0,1,0,0));frame.projection=XMMatrixPerspectiveFovRH(2*std::atan(frame.span/(2*actualDistance)),float(aspect),5,300);
    XMFLOAT4X4 projection;XMStoreFloat4x4(&projection,frame.projection);projection._31+=2*jitter.x/width;projection._32-=2*jitter.y/height;frame.projection=XMLoadFloat4x4(&projection);
    return frame;
}
}
