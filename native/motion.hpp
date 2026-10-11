#pragma once
#include <algorithm>
#include <cmath>
#include <array>
namespace rhine {
inline double smooth(double t){t=std::clamp(t,0.,1.);return t*t*t*(10+t*(-15+6*t));}
inline double themeCurve(double t){t=std::clamp(t,0.,1.);double c=t;for(int i=0;i<6;++i){double x=((1.6*c-1.8)*c+1.2)*c,slope=(4.8*c-3.6)*c+1.2;c=std::clamp(c-(x-t)/slope,0.,1.);}return t==1?1:c*c*(3-2*c);}
inline double bell(double x,double width){return std::exp(-.5*(x/width)*(x/width));}
inline double archiveWave(double row,double lane,double time){double t=time-22,phase=row+(lane-2)*.65;auto packet=[](double d){return 2.5*bell(d,3.8)-.58*bell(d-6,3.5);};return smooth(t/.32)*(packet(phase-3-t*19)*(1-smooth((t-2.15)/.65))+packet(phase-32+(t-2.3)*24)*smooth((t-2.17)/.32)*(1-smooth((t-3.5)/.85)));}
inline double extraction(double time){return .4*smooth((time-25.58)/.82)+2.95*smooth((time-27.55)/1.3);}
inline double settlingWave(double distance,double time=26.56){double age=time-25.05-std::abs(distance)*.065,envelope=std::max(-.42,2.15-.17*(std::sqrt(distance*distance+1)-1)),ring=age>0?std::sin(age*5.1)*std::exp(-age*1.3):0;return envelope*(smooth(age/.62)+.18*ring*smooth(age/.16));}
inline double columnStrength(double lane,double focus,double progress=1){double selected=.25+.75*bell(lane-focus,.55);return 1+(selected-1)*smooth(progress);}
inline double idleWave(double row,double lane,double time){return .075*std::sin(time*6.283185307179586/8+row*.3-lane*.45)+.027*std::sin(time*6.283185307179586/13-row*.17+lane*.3);}
inline double musicSelectionWave(double distance,double age){if(age<0||age>3.2)return 0;return .32*smooth(age/.46)*std::exp(-age*1.35)*std::cos((distance-age*5.5)*.58)*bell(distance-age*5.5,3.4);}
inline double cinematicField(double row,double lane,double time,double center=12,double focus=2){double handoff=smooth((time-24.95)/.45),selection=smooth((time-25.4)/.95),shoulder=time+.3*handoff*(1-selection);return archiveWave(row,lane,time)*(1-handoff)+settlingWave(row-center,shoulder)*columnStrength(lane,focus,(time-25.4)/.95);}
struct Spring{double value=0,velocity=0;void update(double target,double rate,double dt,bool reduced=false){if(reduced){value=target;velocity=0;return;}double delta=value-target,impulse=velocity+rate*delta,decay=std::exp(-rate*dt);value=target+(delta+impulse*dt)*decay;velocity=(velocity-rate*impulse*dt)*decay;}bool settled(double target,double distance=.008,double speed=.025)const{return std::abs(value-target)<distance&&std::abs(velocity)<speed;}};
// Exact upstream polynomial: position, velocity and acceleration survive an
// interrupted extraction. No independent camera/lift/pan tween can diverge.
class Placement {
    double target_=0,elapsed_=1.45/1.05;std::array<double,6> coefficients_{};
public:
    double value=0,velocity=0,acceleration=0;
    static constexpr double duration=1.45/1.05;
    bool settled()const{return elapsed_>=duration;}
    double update(double target,double dt,bool reduced){
        if(reduced){target_=value=target;velocity=acceleration=0;elapsed_=duration;return value;}
        if(target!=target_){target_=target;elapsed_=0;double t=duration,d=target-value-velocity*t-acceleration*t*t/2,v=-velocity-acceleration*t,a=-acceleration;coefficients_={value,velocity,acceleration/2,(10*d-4*v*t+a*t*t/2)/std::pow(t,3),(-15*d+7*v*t-a*t*t)/std::pow(t,4),(6*d-3*v*t+a*t*t/2)/std::pow(t,5)};}
        elapsed_=std::min(duration,elapsed_+std::max(0.,dt));if(settled()){value=target_;velocity=acceleration=0;}else{auto [a,b,c,d,e,f]=coefficients_;double t=elapsed_;value=a+b*t+c*t*t+d*t*t*t+e*std::pow(t,4)+f*std::pow(t,5);velocity=b+2*c*t+3*d*t*t+4*e*t*t*t+5*f*std::pow(t,4);acceleration=2*c+6*d*t+12*e*t*t+20*f*t*t*t;}return value;
    }
};
inline int wrap(int value,int count){if(count<=0)return 0;return (value%count+count)%count;}
inline int nearest(int value,double center,int period){if(period<=0)return value;return value+int(std::floor((center-value+period/2.)/period))*period;}
}
