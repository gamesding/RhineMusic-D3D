void Ui::cdReading(float left,float right,float top,bool iconOnly){
    if(!model_||model_->archive)return;double elapsed=model_->now-model_->cdReadStarted;
    double ending=std::max(model_->cdReadStarted+.7,model_->cdReadFinished);if(!model_->cdReading&&(model_->cdReadFinished<0||model_->now>=ending+.18))return;
    float opacity=model_->preferences.value("reduced",false)?1.f:float(std::min(1.,elapsed/.14));
    if(!model_->cdReading)opacity*=float(std::clamp((ending+.18-model_->now)/.18,0.,1.));if(opacity<=0)return;
    const float textSize=10.f;std::wstring caption=model_->cdReadDrive.empty()?L"正在读取 CD…":wide(model_->cdReadDrive)+L" · 正在读取 CD…";
    float width=iconOnly?27.f:34.f+textWidth(caption,textSize);
    if(right-left<width){if(iconOnly&&right-left<27)return;iconOnly=true;width=27;if(right-left<27){left=width_*.5f-14;right=left+28;top+=49;}}
    float x=(left+right-width)*.5f;D2D1_RECT_F bounds{x,top,x+width,top+27};layoutNode("cd-reading",bounds);
    const std::string key="cd-reading-disc";std::string source;if(!vectors_.contains(key)){std::ifstream file(executableDirectory()/L"runtime/public/native-icons/disc.svg",std::ios::binary);if(!file)throw std::runtime_error("Official CD SVG missing");source.assign(std::istreambuf_iterator<char>(file),{});}
    auto paint=ink_;paint.a*=opacity;float angle=model_->preferences.value("reduced",false)?0:float(std::fmod(elapsed*360.,360.));vector(key,{x,top+1,x+25,top+26},source,paint,angle);
    if(!iconOnly)text(caption,{x+34,top+8,x+width,top+26},textSize,400,paint);
}
