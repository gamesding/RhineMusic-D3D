void Ui::title(Reel& reel,const std::string& raw,D2D1_RECT_F rect,float size){
    const double now=model_->now;const bool reduced=model_->preferences.value("reduced",false);
    const auto value=wide(raw);const float width=std::max(1.f,rect.right-rect.left),budget=std::max(28.f,rect.bottom-rect.top);
    if(reel.title!=value||reel.font!=size||reel.width!=width||reel.budget!=budget){
        const bool animate=!reduced&&!reel.title.empty()&&reel.title!=value;reel.title=value;reel.font=size;reel.width=width;reel.budget=budget;
        std::wstring main=value,translation;
        auto albums=model_->library.value("albums",json::array());
        if(model_->selected>=0&&model_->selected<int(albums.size())){const auto& album=albums[model_->selected];if(album.value("title","")==raw&&album.contains("titleParts")){main=wide(album["titleParts"].value("main",raw));translation=wide(album["titleParts"].value("translation",""));}}
        struct Measured {std::vector<TitleGlyph> glyphs;float height=0;UINT lines=0;};
        auto measure=[&](float chosen,bool clampLines){
            Measured measured;float top=0;
            for(int kind=0;kind<2;++kind){const auto& text=kind==0?main:translation;if(text.empty())continue;const float faceSize=kind==0?chosen:chosen*(&reel==&detailTitle_?.5f:.45f),lineHeight=faceSize*(kind==0?1.4f:1.45f),spacing=kind==0?-.6f:faceSize*.01f;const int weight=kind==0?(&reel==&detailTitle_?600:700):600;
                if(kind)top+=faceSize*.48f;auto* format=font(faceSize,weight);format->SetWordWrapping(DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                Com<IDWriteTextLayout> layout;check(renderer_->writing()->CreateTextLayout(text.c_str(),UINT(text.size()),format,width,4096,&layout),"Original title measurement");Com<IDWriteTextLayout1> advanced;check(layout.As(&advanced),"Native title spacing");check(advanced->SetCharacterSpacing(spacing*.5f,spacing*.5f,0,{0,UINT32_MAX}),"Original title character spacing");check(layout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,lineHeight,faceSize*1.06f),"Original title line height");
                DWRITE_TEXT_METRICS metrics{};check(layout->GetMetrics(&metrics),"Original title bounds");if(kind==0)measured.lines=metrics.lineCount;
                UINT count=0;layout->GetClusterMetrics(nullptr,0,&count);std::vector<DWRITE_CLUSTER_METRICS> clusters(count);check(layout->GetClusterMetrics(clusters.data(),count,&count),"Original title graphemes");UINT position=0;std::map<int,int> rowCounts;
                for(const auto& cluster:clusters){FLOAT x=0,y=0;DWRITE_HIT_TEST_METRICS hit{};check(layout->HitTestTextPosition(position,FALSE,&x,&y,&hit),"Original title glyph bounds");const int line=int(std::round(y/lineHeight));auto ink=text.substr(position,cluster.length);position+=cluster.length;if(cluster.isNewline||cluster.width==0||clampLines&&line>=2)continue;TitleGlyph glyph;glyph.key=std::string(kind?"translation:":"main:")+std::to_string(line)+":"+std::to_string(rowCounts[line]++);glyph.face={ink,faceSize,lineHeight,cluster.width,weight,spacing,bool(kind)};glyph.x=x;glyph.y=top+line*lineHeight;measured.glyphs.push_back(std::move(glyph));}
                if(clampLines&&metrics.lineCount>2){auto last=measured.glyphs.end();while(last!=measured.glyphs.begin()){--last;if(last->face.translation==bool(kind)&&last->y>=top+lineHeight-.01f)break;}if(last!=measured.glyphs.end()){last->face.text=L"\u2026";}}
                top+=lineHeight*(clampLines?std::min(2u,metrics.lineCount):metrics.lineCount);
            }
            measured.height=std::min(top,budget);return measured;
        };
        auto measured=measure(size,false);const float minimum=std::min(size,width_<700?18.f:20.f);
        if(measured.lines>2||measured.height>=budget){float previous=size;bool found=false;for(int step=1;step<=8;++step){const float candidate=size-(size-minimum)*step/8;auto attempt=measure(candidate,false);if(attempt.lines<=2&&attempt.height<budget){float low=candidate,high=previous;for(int fine=1;fine<=16;++fine){auto chosen=measure(high-(high-low)*fine/16,false);if(chosen.lines<=2&&chosen.height<budget){measured=std::move(chosen);found=true;break;}}break;}previous=candidate;}if(!found)measured=measure(minimum,true);}
        reel.apply(measured.glyphs,measured.height,now,animate);
    }
    drawReel(reel,rect,ink_);
}
void Ui::drawReel(Reel& reel,D2D1_RECT_F rect,D2D1_COLOR_F paint){
    if(measuring_)return;
    const double now=model_->now;const bool reduced=model_->preferences.value("reduced",false);
    reel.collect(now,reduced);auto drawing=renderer_->drawing();
    for(auto& [key,slot]:reel.slots){if(!slot.active&&slot.roll.finished(now))continue;const float x=rect.left+float(slot.x.sample(now).value),y=rect.top+float(slot.y.sample(now).value),height=float(slot.viewport.sample(now).value),roll=float(slot.roll.sample(now).value);float width=slot.glyph.face.width;for(const auto& face:slot.faces)width=std::max(width,face.width);const float pad=slot.glyph.face.font*.15f;
        auto clip=D2D1::RectF(x-pad,y,x+width+pad+.3f,y+height);drawing->PushAxisAlignedClip(clip,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        const bool moving=!slot.roll.finished(now)||!slot.viewport.finished(now);Com<ID2D1LinearGradientBrush> mask;
        if(moving){const float edge=std::min(.49f,slot.glyph.face.font*.02f/std::max(1.f,height));D2D1_GRADIENT_STOP stops[]={{0,D2D1::ColorF(1,1,1,0)},{edge,D2D1::ColorF(1,1,1,1)},{1-edge,D2D1::ColorF(1,1,1,1)},{1,D2D1::ColorF(1,1,1,0)}};Com<ID2D1GradientStopCollection> gradient;check(drawing->CreateGradientStopCollection(stops,4,&gradient),"Title reel fade stops");check(drawing->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties({x,y},{x,y+height}),gradient.Get(),&mask),"Title reel fade");D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds=clip;layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.maskAntialiasMode=D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;layer.opacity=1;layer.opacityBrush=mask.Get();drawing->PushLayer(layer,nullptr);}
        for(size_t i=0;i<slot.faces.size();++i){const auto& face=slot.faces[i];const float faceY=y+slot.offsets[i]-roll;if(face.text.empty()||faceY+face.height<y||faceY>y+height)continue;drawing->PushAxisAlignedClip(D2D1::RectF(x-pad,faceY,x+width+pad+.3f,faceY+face.height),D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);text(face.text,{x,faceY,x+std::max(width,face.font*2),faceY+face.height},face.font,face.weight,face.translation?muted_:paint,false,false,0,face.height/face.font);drawing->PopAxisAlignedClip();}
        if(moving)drawing->PopLayer();drawing->PopAxisAlignedClip();
    }
}
