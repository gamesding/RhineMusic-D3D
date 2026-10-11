#pragma once
#include "scene.hpp"
namespace rhine {
// The original viewer uses spherical orbit, logarithmic zoom, and independent
// focus damping. Reset interruption captures the rendered pose, not its goal.
class Viewer {
    struct Pose {double radius=14.501034445,phi=1.305650863,theta=.5404195;};
    Pose pose_{},goal_{},resetFrom_{};XMFLOAT3 focus_{},target_{},resetFocus_{};
    Spring spread_{},clarity_{1};double resetElapsed_=0;bool resetting_=false,first_=true;
    std::vector<XMFLOAT4X4> previous_;
    void interrupt(){if(resetting_){goal_=pose_;target_=focus_;resetting_=false;}}
    static Pose initial(){const double radius=std::sqrt(7.2*7.2+3.8*3.8+12*12);return {radius,std::acos(3.8/radius),std::atan2(7.2,12.)};}
public:
    bool opened=false,exploded=false,clear=true;int album=-1;
    void open(int selected){opened=true;album=selected;exploded=false;clear=true;spread_={};clarity_={1};pose_=goal_=initial();focus_=target_={};resetting_=false;first_=true;previous_.clear();}
    void close(){opened=false;}
    void resetRendering(){first_=true;previous_.clear();}
    void orbit(double dx,double dy,int height){interrupt();const double rate=6.283185307179586*.65/std::max(1,height);goal_.theta-=dx*rate;goal_.phi=std::clamp(goal_.phi-dy*rate,.00001,3.141582653589793);}
    void zoom(double pixels){interrupt();goal_.radius=std::clamp(goal_.radius*std::exp(pixels*.0014),5.,28.);}
    void pan(double x,double y,bool pixels=false,int height=1080){interrupt();auto d=XMVectorSet(float(std::sin(pose_.phi)*std::sin(pose_.theta)),float(std::cos(pose_.phi)),float(std::sin(pose_.phi)*std::cos(pose_.theta)),0);auto right=XMVector3Normalize(XMVector3Cross(XMVectorSet(0,1,0,0),d)),up=XMVector3Normalize(XMVector3Cross(d,right));const double rate=pixels?2*goal_.radius*std::tan(17*XM_PI/180)/std::max(1,height):goal_.radius*.025;auto t=XMLoadFloat3(&target_)+(right*float(x)+up*float(y))*float(rate);const float length=XMVectorGetX(XMVector3Length(t));if(length>5)t*=5/length;XMStoreFloat3(&target_,t);}
    void reset(){resetFrom_=pose_;resetFocus_=focus_;goal_=initial();target_={};resetElapsed_=0;resetting_=true;}
    SceneFrame update(double dt,unsigned width,unsigned height,bool reduced,float night,XMFLOAT2 jitter,const std::vector<std::pair<int,float>>& parts){
        dt=std::clamp(dt,0.,.05);double pan=1-std::exp(-13*dt),orbit=1-std::exp(-9*dt),zoom=1-std::exp(-15*dt);Pose from=pose_;XMFLOAT3 focusFrom=focus_;
        if(reduced){from=goal_;focusFrom=target_;pan=orbit=zoom=1;resetting_=false;}
        else if(resetting_){resetElapsed_+=dt;pan=orbit=zoom=1-std::pow(1-std::min(1.,resetElapsed_/.56),3);from=resetFrom_;focusFrom=resetFocus_;if(resetElapsed_>=.56)resetting_=false;}
        pose_.radius=std::exp(std::lerp(std::log(from.radius),std::log(goal_.radius),zoom));pose_.phi=std::lerp(from.phi,goal_.phi,orbit);pose_.theta=from.theta+std::atan2(std::sin(goal_.theta-from.theta),std::cos(goal_.theta-from.theta))*orbit;
        focus_={float(std::lerp(focusFrom.x,target_.x,pan)),float(std::lerp(focusFrom.y,target_.y,pan)),float(std::lerp(focusFrom.z,target_.z,pan))};
        spread_.update(exploded?1:0,5.5,dt,reduced);clarity_.update(clear?1:0,8,dt,reduced);
        SceneFrame frame;frame.reduced=reduced;frame.viewer=true;frame.reset=first_;frame.night=night;frame.detail=1;frame.focus=float(pose_.radius);frame.span=2*frame.focus*std::tan(17*XM_PI/180);frame.nearPlane=.3f;frame.farPlane=120;
        frame.camera={focus_.x+float(pose_.radius*std::sin(pose_.phi)*std::sin(pose_.theta)),focus_.y+float(pose_.radius*std::cos(pose_.phi)),focus_.z+float(pose_.radius*std::sin(pose_.phi)*std::cos(pose_.theta))};frame.aim=focus_;
        frame.view=XMMatrixLookAtRH(XMLoadFloat3(&frame.camera),XMLoadFloat3(&focus_),XMVectorSet(0,1,0,0));frame.projection=XMMatrixPerspectiveFovRH(34*XM_PI/180,float(width)/height,.3f,120);
        XMFLOAT4X4 projection;XMStoreFloat4x4(&projection,frame.projection);projection._31+=2*jitter.x/width;projection._32-=2*jitter.y/height;frame.projection=XMLoadFloat4x4(&projection);
        for(size_t i=0;i<parts.size();++i){Instance instance;XMStoreFloat4x4(&instance.world,XMMatrixTranslation(0,-1.85f,float(parts[i].second*spread_.value)));instance.previous=i<previous_.size()?previous_[i]:instance.world;instance.uv={0,0,1,1};instance.data={0,float(clarity_.value),0,float(parts[i].first)};frame.instances.push_back(instance);frame.albums.push_back(album);}
        previous_.clear();for(auto& instance:frame.instances)previous_.push_back(instance.world);first_=false;return frame;
    }
    json status()const{return {{"open",opened},{"exploded",exploded},{"clear",clear},{"spread",spread_.value},{"clarity",clarity_.value},{"cameraRadius",pose_.radius},{"resetting",resetting_}};}
};
}
