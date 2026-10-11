float Ui::badgePhase(const std::string& name,bool track,double duration){
    std::string id;if(model_->selected>=0&&model_->selected<int(arrayField(model_->library,"albums").size()))id=arrayField(model_->library,"albums")[model_->selected].value("id",std::string());
    auto key=name+(track?"-track":model_->detailSurface?"-detail":"-browse");auto& clock=badgeClocks_[key];if(clock.identity!=id||model_->now-clock.seen>.2){clock.identity=id;clock.started=model_->now;}clock.seen=model_->now;
    return model_->preferences.value("reduced",false)?.5625f:float(std::fmod(model_->now-clock.started,duration)/duration);
}
void Ui::ensureBadgeMask(){
    auto drawing=renderer_->drawing();Com<ID2D1Image> previous;drawing->GetTarget(&previous);float oldScale=scale_;scale_=1;
    const std::array<std::tuple<const char*,unsigned,unsigned>,5> definitions{{{"dsd",500,254},{"hi-res",256,256},{"dsee",512,144},{"s-master",480,72},{"vinyl",256,256}}};
    auto target=[&](const std::string& key,unsigned w,unsigned h){auto& bitmap=badgeMasks_[key];if(!bitmap)check(drawing->CreateBitmap({w,h},nullptr,0,D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED)),&bitmap),"Original badge bitmap");drawing->SetTarget(bitmap.Get());drawing->SetTransform(D2D1::Matrix3x2F::Identity());drawing->Clear(color(0,0));};
    for(auto [name,w,h]:definitions)if(!badgeMasks_[name]){target(name,w,h);logo(name,{0,0,float(w),float(h)},color(0xffffff));}
    bool hires=model_->selected>=0&&model_->selected<int(arrayField(model_->library,"albums").size())&&arrayField(model_->library,"albums")[model_->selected].value("hiRes",false);
    if(hires&&!model_->booting&&!model_->archive&&!model_->viewer){
        if(arrayField(model_->library,"albums")[model_->selected].value("dsd",false)){badgePhase("dsd",false,8.4);if(model_->detailSurface)badgePhase("dsd",true,12);}
        auto& shine=gradients_["hi-res-composite-shine"];
        if(!shine){const D2D1_GRADIENT_STOP stops[]={{0,color(0xfff8dc,0)},{.43f,color(0xfff8dc,0)},{.46f,color(0xfff8dc,.12f)},{.49f,color(0xfffbeb,.85f)},{.52f,color(0xfff3c7,.2f)},{.55f,color(0xfff3c7,0)},{1,color(0xfff3c7,0)}};Com<ID2D1GradientStopCollection> collection;check(drawing->CreateGradientStopCollection(stops,7,&collection),"Original screen sheen");check(drawing->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({0,0},{1,0}),collection.Get(),&shine),"Original sheen brush");}
        for(bool track:{false,true}){if(track&&!model_->detailSurface)continue;std::string name=track?"hi-res-screen-track":"hi-res-screen-album",sheen=name+"-input";target(sheen,256,256);bool reduced=model_->preferences.value("reduced",false);float phase=badgePhase("hi-res",track,track?9.6:6.8),center=128+640*(-.64f+1.28f*phase),length=.92718385f*640+.37460659f*460.8f;shine->SetStartPoint({center-.92718385f*length*.5f,128-.37460659f*length*.5f});shine->SetEndPoint({center+.92718385f*length*.5f,128+.37460659f*length*.5f});shine->SetOpacity(reduced?.25f:track?.55f:1.f);drawing->FillRectangle({0,0,256,256},shine.Get());
            auto& effect=badgeEffects_[name];if(!effect){check(drawing->CreateEffect(CLSID_D2D1Blend,&effect),"Original Hi-Res screen blend");check(effect->SetValue(D2D1_BLEND_PROP_MODE,D2D1_BLEND_MODE_SCREEN),"Original screen blend mode");effect->SetInput(0,badgeMasks_["hi-res"].Get());}effect->SetInput(1,badgeMasks_[sheen].Get());
        }
        float hi=std::clamp(width_*.035f,40.f,54.f),ratio=256/hi;auto& halo=badgeMasks_["hi-res-halo-mask"];
        if(!halo||haloMaskWidth_!=hi){haloMaskWidth_=hi;target("hi-res-halo-mask",256,256);brush_->SetColor(color(0xffffff));drawing->DrawRoundedRectangle(D2D1::RoundedRect({ratio*.5f,ratio*.5f,256-ratio*.5f,256-ratio*.5f},ratio*2,ratio*2),brush_.Get(),ratio);}
        for(auto name:{"hi-res-shadow-day","hi-res-shadow-night","hi-res-halo-outer","hi-res-halo-inner"}){auto& effect=badgeEffects_[name];if(!effect){check(drawing->CreateEffect(CLSID_D2D1Shadow,&effect),"Original badge shadow");effect->SetInput(0,std::string(name).find("halo")!=std::string::npos?badgeMasks_["hi-res-halo-mask"].Get():badgeMasks_["hi-res"].Get());}bool day=std::string(name)=="hi-res-shadow-day",night=std::string(name)=="hi-res-shadow-night",inner=std::string(name)=="hi-res-halo-inner";float blur=day?1.5f:night?5.f:inner?2.5f:std::lerp(6.f,8.f,model_->night);auto tint=day?color(0x725018,.18f):night?color(0xddc77c,.15f):inner?color(0xfff2be,std::lerp(.16f,.18f,model_->night)):blend(color(0xb4802c,.14f),color(0xddc77c,.17f),model_->night);check(effect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION,blur*ratio),"Original halo blur");check(effect->SetValue(D2D1_SHADOW_PROP_COLOR,D2D1::Vector4F(tint.r,tint.g,tint.b,tint.a)),"Original halo color");}
    }
    scale_=oldScale;drawing->SetTarget(previous.Get());drawing->SetTransform(D2D1::Matrix3x2F::Scale(scale_,scale_));
}

void Ui::audioBadge(const std::string& name,D2D1_RECT_F rect,bool track){
    if(measuring_)return;
    const bool dsd=name=="dsd",reduced=model_->preferences.value("reduced",false);
    auto paint=dsd?blend(color(0x333a32),color(0xe9eef2),model_->night):color(0xffffff);
    if(dsd)logo(name,rect,paint);
    float w=rect.right-rect.left,h=rect.bottom-rect.top;
    double duration=dsd?(track?12.:8.4):(track?9.6:6.8);
    float phase=badgePhase(name,track,duration);
    badgeEvidence_.push_back({{"name",name},{"track",track},{"duration",duration},{"phase",phase},{"linear",true},{"reduced",reduced},{"rect",json::array({rect.left,rect.top,rect.right,rect.bottom})}});
    // CSS inset -40% -75% (DSD -70%), translateX(-64%..64%), 112 degrees.
    float pseudoWidth=w*(dsd?2.4f:2.5f),pseudoHeight=h*1.8f;
    float translate=pseudoWidth*(-.64f+1.28f*phase);
    float center=(rect.left+rect.right)*.5f+translate;
    float length=std::abs(.92718385f*pseudoWidth)+std::abs(-.37460659f*pseudoHeight);
    auto& shine=gradients_[dsd?"dsd-sheen":"hi-res-sheen"];
    if(!shine){Com<ID2D1GradientStopCollection> stops;
        const D2D1_GRADIENT_STOP gold[]={{0,color(0xb4802c,0)},{.40f,color(0xb4802c,0)},{.46f,color(0xb4802c)},{.50f,color(0xfff3ce)},{.54f,color(0xddc77c)},{.60f,color(0xddc77c,0)},{1,color(0xddc77c,0)}};
        const D2D1_GRADIENT_STOP white[]={{0,color(0xfff8dc,0)},{.43f,color(0xfff8dc,0)},{.46f,color(0xfff8dc,.12f)},{.49f,color(0xfffbeb,.85f)},{.52f,color(0xfff3c7,.2f)},{.55f,color(0xfff3c7,0)},{1,color(0xfff3c7,0)}};
        check(renderer_->drawing()->CreateGradientStopCollection(dsd?gold:white,7,&stops),"Original badge sheen stops");
        check(renderer_->drawing()->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({0,0},{1,0}),stops.Get(),&shine),"Original badge sheen");
    }
    float cy=(rect.top+rect.bottom)*.5f;
    shine->SetStartPoint({center-.92718385f*length*.5f,cy-.37460659f*length*.5f});
    shine->SetEndPoint({center+.92718385f*length*.5f,cy+.37460659f*length*.5f});
    shine->SetOpacity(reduced?(dsd?.2f:.25f):(dsd?(track?.65f:.8f):(track?.55f:1.f)));
    auto drawing=renderer_->drawing();
    if(dsd){
        // The supplied silhouette, including all lettering, is the opacity mask.
        auto& mask=badgeMasks_[name];
        drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);drawing->FillOpacityMask(mask.Get(),shine.Get(),rect,D2D1::RectF(0,0,500,253.76521f));drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }else{
        auto drawEffect=[&](const char* key,float opacity,float offset=0.f){auto& effect=badgeEffects_[key];if(!effect||opacity<=.0001f)return;D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds={0,0,width_,height_};layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.opacity=opacity;drawing->PushLayer(layer,nullptr);drawing->SetTransform(D2D1::Matrix3x2F::Scale(w/256,h/256)*D2D1::Matrix3x2F::Translation(rect.left,rect.top+offset)*D2D1::Matrix3x2F::Scale(scale_,scale_));drawing->DrawImage(effect.Get());drawing->SetTransform(D2D1::Matrix3x2F::Scale(scale_,scale_));drawing->PopLayer();};
        if(!track){drawEffect("hi-res-shadow-day",1-model_->night,2);drawEffect("hi-res-shadow-night",model_->night);}
        drawEffect(track?"hi-res-screen-track":"hi-res-screen-album",1);
        if(!track){float breath=reduced?1.f:(phase<.5f?.5f+phase:1.5f-phase);drawEffect("hi-res-halo-outer",breath);drawing->PushAxisAlignedClip(rect,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);drawEffect("hi-res-halo-inner",breath);drawing->PopAxisAlignedClip();auto halo=blend(color(0xddc77c,.6f),color(0xf9e1b4,.72f),model_->night);halo.a*=breath;brush_->SetColor(halo);drawing->DrawRoundedRectangle(D2D1::RoundedRect({rect.left-.5f,rect.top-.5f,rect.right+.5f,rect.bottom+.5f},1.5f,1.5f),brush_.Get(),1);}

    }
}
float Ui::albumBadges(const json& album,float left,float right,float top,float size){
    if(!album.value("hiRes",false))return right;
    float hi=std::clamp(width_*.035f,40.f,54.f),dsd=std::clamp(width_*.048f,58.f,76.f),gap=portrait_?14.f:18.f;
    float occupied=hi+(album.value("dsd",false)?dsd+10:0);
    float x=right-occupied;
    audioBadge("hi-res",{x,top+size*.14f,x+hi,top+size*.14f+hi},false);
    if(album.value("dsd",false))audioBadge("dsd",{x+hi+10,top+size*.14f+(hi-dsd*.50753f)*.5f,right,top+size*.14f+(hi+dsd*.50753f)*.5f},false);
    return std::max(left+20,right-occupied-gap);
}

float Ui::sonyBadge(const std::string& name,D2D1_RECT_F rect,const std::wstring& caption,bool playing){
    if(measuring_)return rect.right-rect.left+9+textWidth(caption,8)+caption.size()*1.15f;
    bool reduced=model_->preferences.value("reduced",false);auto& clock=techClocks_[name];if(model_->now-clock.seen>.2){clock.elapsed=0;clock.appeared=model_->now;}else if(clock.playing)clock.elapsed+=model_->now-clock.seen;clock.seen=model_->now;clock.playing=playing;
    float appear=reduced?1.f:float(std::clamp((model_->now-clock.appeared)/.42,0.,1.));float offset=reduced?0.f:(1-appear)*4;rect.top+=offset;rect.bottom+=offset;float w=rect.right-rect.left,h=rect.bottom-rect.top;
    auto drawing=renderer_->drawing();D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds={0,0,width_,height_};layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.opacity=appear;drawing->PushLayer(layer,nullptr);
    if(name=="vinyl")drawing->SetTransform(D2D1::Matrix3x2F::Rotation(reduced?0.f:float(std::fmod(clock.elapsed,48)/48*360),{(rect.left+rect.right)*.5f,(rect.top+rect.bottom)*.5f})*D2D1::Matrix3x2F::Scale(scale_,scale_));
    auto tint=ink_;tint.a=.76f;auto& mask=badgeMasks_[name];brush_->SetColor(tint);drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);drawing->FillOpacityMask(mask.Get(),brush_.Get(),rect);drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    for(int i=0;i<2;++i){auto key=i?"sony-tech-shine-night":"sony-tech-shine-day";auto& shine=gradients_[key];if(!shine){const D2D1_GRADIENT_STOP stops[]={{0,color(0xffffff,0)},{.35f,color(0xffffff,0)},{.5f,color(i?0x162234:0xefede7)},{.65f,color(0xffffff,0)},{1,color(0xffffff,0)}};Com<ID2D1GradientStopCollection> collection;check(drawing->CreateGradientStopCollection(stops,5,&collection),"Original Sony badge light");check(drawing->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({0,0},{1,0}),collection.Get(),&shine),"Original Sony linear light");}
        float phase=reduced?0.f:float(std::fmod(clock.elapsed,12)/12),pw=w*2.2f,ph=h*1.6f,cx=(rect.left+rect.right)*.5f+pw*(-1.1f+2.2f*phase),cy=(rect.top+rect.bottom)*.5f,length=.93969262f*pw+.34202014f*ph;shine->SetStartPoint({cx-.93969262f*length*.5f,cy-.34202014f*length*.5f});shine->SetEndPoint({cx+.93969262f*length*.5f,cy+.34202014f*length*.5f});shine->SetOpacity(.22f*.76f*(i?model_->night:1-model_->night));drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);drawing->FillOpacityMask(mask.Get(),shine.Get(),rect);drawing->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }float phase=reduced?0.f:float(std::fmod(clock.elapsed,12)/12);drawing->SetTransform(D2D1::Matrix3x2F::Scale(scale_,scale_));
    auto copy=ink_;copy.a=.65f;float textY=(rect.top+rect.bottom)*.5f-5.6f;float captionWidth=textWidth(caption,8)+caption.size()*1.15f;text(caption,{rect.right+9,textY,rect.right+9+captionWidth,rect.bottom+10},8,400,copy,false,false,1.15f);drawing->PopLayer();
    badgeEvidence_.push_back({{"name",name},{"track",false},{"duration",name=="vinyl"?48:12},{"phase",name=="vinyl"?std::fmod(clock.elapsed,48)/48:phase},{"linear",true},{"playing",playing},{"reduced",reduced},{"rect",json::array({rect.left,rect.top,rect.right,rect.bottom})}});return w+9+captionWidth;
}
