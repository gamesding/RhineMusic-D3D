#pragma once
#include <algorithm>
namespace rhine {
struct ViewportLayout {float width,height,scale;bool portrait,compact;};
inline ViewportLayout viewportLayout(unsigned physicalWidth,unsigned physicalHeight,float dpi=1,bool coarse=false){
    dpi=std::max(.5f,dpi);const float cssWidth=physicalWidth/dpi,cssHeight=physicalHeight/dpi;
    const bool portrait=cssWidth/std::max(1.f,cssHeight)<1.05f;
    const bool compact=portrait||cssWidth<1100||(coarse&&cssHeight<600);
    const float scale=compact?dpi:float(physicalHeight)/1080;
    return {physicalWidth/scale,physicalHeight/scale,scale,portrait,compact};
}
struct PlaybackLayout {float left,top,right,bottom,gap;bool portrait,compact,utilities;};
inline PlaybackLayout playbackLayout(float width,float height){
    const bool portrait=width/height<1.05f,compact=width<1100;
    const float left=portrait?std::clamp(width*.04f,22.f,42.f):compact?24.f:std::max(42.f,(width-1280)/2);
    const float right=width-(portrait?std::clamp(width*.04f,18.f,42.f):left);
    const float bottom=height-32,top=bottom-(portrait?142.f:width<950?114.f:80.f);
    return {left,top,right,bottom,portrait?10.f:compact?14.f:20.f,portrait,compact,!portrait||width>=560};
}
}
