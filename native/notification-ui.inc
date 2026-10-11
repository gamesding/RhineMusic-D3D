void Ui::notification(){
    notificationEvidence_=json::object();notificationVisible_=false;
    if(model_->message.empty())return;
    if(notificationIdentity_!=model_->message||notificationStarted_!=model_->messageTime){
        notificationIdentity_=model_->message;notificationStarted_=model_->messageTime;
        notificationScroll_=notificationScrollMax_=notificationHold_=0;notificationPointer_=false;
    }
    if(notificationScrollMax_>0&&(notificationPointer_||contains(notificationBounds_,pointerX_,pointerY_)))notificationHold_+=frameDt_;
    float elapsed=float(model_->now-model_->messageTime)-notificationHold_;
    if(elapsed>=5.7f)return;
    const float size=width_<=700?10.f:11.f,padX=18,padY=12;
    const float maxWidth=std::min(580.f,width_*.85f),contentWidth=maxWidth-2*padX;
    const auto value=wide(model_->message);
    const auto key=L"notification|"+std::to_wstring(size)+L"|"+std::to_wstring(contentWidth)+L"|"+value;
    auto found=textCache_.find(key);
    if(found==textCache_.end()){
        if(textCache_.size()>=1024)textCache_.clear();CachedText cached;
        check(renderer_->writing()->CreateTextLayout(value.c_str(),UINT(value.size()),font(size),contentWidth,10000,&cached.layout),"Notification layout");
        cached.layout->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
        cached.layout->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        cached.layout->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        cached.layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,size*1.7f,size*1.21f);
        check(cached.layout->GetMetrics(&cached.metrics),"Notification measure");
        // Round up and retain a two-pixel margin at the wrap boundary. The
        // exact same layout is drawn, so a second measurement cannot disagree.
        cached.layout->SetMaxWidth(std::min(contentWidth,std::ceil(cached.metrics.widthIncludingTrailingWhitespace)+2));
        check(cached.layout->GetMetrics(&cached.metrics),"Notification final measure");
        found=textCache_.emplace(key,std::move(cached)).first;
    }
    const auto& cached=found->second;DWRITE_OVERHANG_METRICS overhang{};
    check(cached.layout->GetOverhangMetrics(&overhang),"Notification glyph bounds");
    const float w=cached.layout->GetMaxWidth()+2*padX;
    float bottom=height_-30,left=(width_-w)*.5f,right=left+w;
    for(const auto* name:{"playback-glass","music-navigation-occupied","music-navigation-hint","cd-eject"}){
        auto node=layoutNodes_.find(name);if(node==layoutNodes_.end())continue;
        const auto& box=node.value();if(left<box[0].get<float>()+box[2].get<float>()&&right>box[0].get<float>())bottom=std::min(bottom,box[1].get<float>()-12);
    }
    bottom=std::max(70.f,bottom);
    const float contentHeight=std::ceil(cached.metrics.height+std::max(0.f,overhang.bottom))+2;
    const float viewportHeight=std::min(contentHeight,std::max(24.f,bottom-16-2*padY));
    notificationScrollMax_=std::max(0.f,contentHeight-viewportHeight);
    notificationScroll_=std::clamp(notificationScroll_,0.f,notificationScrollMax_);
    const float h=viewportHeight+2*padY,opacity=model_->preferences.value("reduced",false)?1.f:std::min(std::min(1.f,std::max(0.f,elapsed)/.2f),std::clamp((5.7f-elapsed)/.2f,0.f,1.f));
    const float top=bottom-h+(1-opacity)*8;
    notificationBounds_={left,top,right,top+h};notificationVisible_=true;
    layoutNode("notification",notificationBounds_);
    auto bg=ink_;bg.a*=opacity;fill(notificationBounds_,bg);
    auto fg=blend(color(0xefede7),color(0x162234),model_->night);fg.a*=opacity;
    D2D1_RECT_F inner{left+padX,top+padY,right-padX,top+padY+viewportHeight};
    auto drawing=renderer_->drawing();drawing->PushAxisAlignedClip(inner,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    brush_->SetColor(fg);drawing->DrawTextLayout({inner.left,inner.top-notificationScroll_},cached.layout.Get(),brush_.Get());drawing->PopAxisAlignedClip();
    if(notificationScrollMax_>0){const float track=viewportHeight,thumb=std::max(12.f,track*viewportHeight/contentHeight),y=inner.top+(track-thumb)*notificationScroll_/notificationScrollMax_;auto trackColor=fg;trackColor.a*=.5f;fill({right-8,y,right-5,y+thumb},trackColor,1.5f);}
    notificationEvidence_={{"text",model_->message},{"lineCount",cached.metrics.lineCount},{"textWidth",cached.metrics.widthIncludingTrailingWhitespace},{"layoutWidth",cached.layout->GetMaxWidth()},{"contentHeight",contentHeight},{"viewportHeight",viewportHeight},{"scroll",notificationScroll_},{"scrollMax",notificationScrollMax_},{"opacity",opacity},{"elapsed",elapsed},{"fullyVisible",notificationScrollMax_==0},{"bounds",json::array({left,top,w,h})}};
}
