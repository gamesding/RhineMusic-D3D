float Ui::movingText(const std::string& key,const std::wstring& value,D2D1_RECT_F rect,float size,int weight,D2D1_COLOR_F paint,float spacing,float lineHeight,bool wrap){
    const float width=std::max(1.f,rect.right-rect.left),line=size*lineHeight;auto& reel=textReels_[key];
    if(wrap&&textWidth(value,size,weight)+spacing*value.size()>width)return text(value,rect,size,weight,paint,true,false,spacing,lineHeight);
    if(reel.title!=value||reel.font!=size||reel.width!=width||reel.budget!=line){
        bool animate=!model_->preferences.value("reduced",false)&&!reel.title.empty()&&reel.title!=value;reel.title=value;reel.font=size;reel.width=width;reel.budget=line;
        auto format=font(size,weight);format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,line,size*(1.06f+(lineHeight-1.4f)*.5f));
        Com<IDWriteTextLayout> layout;check(renderer_->writing()->CreateTextLayout(value.c_str(),UINT(value.size()),format,width,4096,&layout),"Original metadata reel measure");
        if(spacing){Com<IDWriteTextLayout1> advanced;check(layout.As(&advanced),"Metadata reel spacing");advanced->SetCharacterSpacing(spacing*.5f,spacing*.5f,0,{0,UINT32_MAX});}
        UINT count=0;layout->GetClusterMetrics(nullptr,0,&count);std::vector<DWRITE_CLUSTER_METRICS> clusters(count);if(count)check(layout->GetClusterMetrics(clusters.data(),count,&count),"Metadata graphemes");
        UINT position=0;std::vector<TitleGlyph> glyphs;for(size_t n=0;n<clusters.size();++n){const auto& cluster=clusters[n];FLOAT x=0,y=0;DWRITE_HIT_TEST_METRICS hit{};layout->HitTestTextPosition(position,FALSE,&x,&y,&hit);auto ink=value.substr(position,cluster.length);position+=cluster.length;if(cluster.isNewline||!cluster.width)continue;glyphs.push_back({std::to_string(n),{ink,size,line,cluster.width,weight,spacing,false},x,0});}
        reel.apply(glyphs,line,model_->now,animate);
    }
    reel.collect(model_->now,model_->preferences.value("reduced",false));drawReel(reel,rect,paint);return reel.height;
}
void Ui::flatButton(std::string key,std::wstring label,D2D1_RECT_F rect,std::string action,json args,float size,D2D1_COLOR_F paint){
    Hit hit{key,std::move(action),std::move(args),rect,false,""};if(detailClipActive_)hit.clip=detailHitClip_;hits_.push_back(std::move(hit));
    if(contains(rect,pointerX_,pointerY_)||focus_==key)paint=accent_;
    if(focus_==key){brush_->SetColor(accent_);renderer_->drawing()->DrawRectangle({rect.left-3,rect.top-3,rect.right+3,rect.bottom+3},brush_.Get(),1);}
    text(std::move(label),{rect.left,rect.top+(rect.bottom-rect.top-size*1.3f)*.5f,rect.right,rect.bottom},size,400,paint,false,false,0,1.3f);
}
