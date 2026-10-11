static double panelCurve(double x,bool opening){
    x=std::clamp(x,0.,1.);double t=x,x1=opening?.22:.4,x2=opening?.36:1.,y1=opening?1.:0.;
    for(int i=0;i<8;++i){double q=1-t,at=3*q*q*t*x1+3*q*t*t*x2+t*t*t,d=3*q*q*x1+6*q*t*(x2-x1)+3*t*t*(1-x2);if(d<1e-8)break;t=std::clamp(t-(at-x)/d,0.,1.);}
    return 3*(1-t)*(1-t)*t*y1+3*(1-t)*t*t+t*t*t;
}
float Ui::paragraph(std::wstring value,float left,float right,float y,float size,float lineHeight){return text(std::move(value),{left,y,right,y+10000},size,400,muted_,true,false,0,lineHeight);}
float Ui::textWidth(const std::wstring& value,float size,int weight){
    auto key=L"measure|"+std::to_wstring(size)+L"|"+std::to_wstring(weight)+L"|"+value;auto found=textCache_.find(key);
    if(found==textCache_.end()){if(textCache_.size()>=1024)textCache_.clear();CachedText cached;auto format=font(size,weight);format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);check(renderer_->writing()->CreateTextLayout(value.c_str(),UINT(value.size()),format,10000,10000,&cached.layout),"Original control text measure");check(cached.layout->GetMetrics(&cached.metrics),"Original control width");found=textCache_.emplace(std::move(key),std::move(cached)).first;}return found->second.metrics.widthIncludingTrailingWhitespace;
}
void Ui::panelButton(std::string key,std::wstring value,D2D1_RECT_F rect,std::string action,json args,bool primary,bool disabled,float size){
    bool hovered=contains(rect,pointerX_,pointerY_)&&contains(panelClip_,pointerX_,pointerY_);
    if(primary)fill(rect,disabled?blend(ink_,panelColor_,.55f):ink_);
    auto paint=disabled?muted_:primary?blend(color(0xefede7),color(0x162234),model_->night):accent_;
    if((key=="library-rescan"||hovered)&&!disabled&&!primary)paint=ink_;
    text(std::move(value),{rect.left+(primary?18:0),rect.top+(primary?11:0),rect.right,rect.bottom},size,400,paint);
    if(focus_==key&&focusVisible_){brush_->SetColor(accent_);renderer_->drawing()->DrawRectangle({rect.left-5,rect.top-5,rect.right+5,rect.bottom+5},brush_.Get(),2);}
    Hit hit{std::move(key),std::move(action),std::move(args),rect,disabled,""};if(panelClipActive_)hit.clip=panelClip_;hits_.push_back(std::move(hit));
}
float Ui::textHeight(const std::wstring& value,float size,float width,float lineHeight){
    auto key=L"height|"+std::to_wstring(size)+L"|"+std::to_wstring(width)+L"|"+std::to_wstring(lineHeight)+L"|"+value;auto found=textCache_.find(key);
    if(found==textCache_.end()){if(textCache_.size()>=1024)textCache_.clear();CachedText cached;auto format=font(size);format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM,size*lineHeight,size*(1.06f+(lineHeight-1.4f)*.5f));check(renderer_->writing()->CreateTextLayout(value.c_str(),UINT(value.size()),format,std::max(1.f,width),10000,&cached.layout),"Original multiline text measure");check(cached.layout->GetMetrics(&cached.metrics),"Original multiline height");found=textCache_.emplace(std::move(key),std::move(cached)).first;}return found->second.metrics.height;
}
void Ui::selectControl(std::string key,D2D1_RECT_F rect,const json& values,const json& current,std::string group,bool disabled){
    std::wstring name=label(current);for(auto& item:values)if(item.at("value")==current)name=wide(item.at("label").get<std::string>());
    fill(rect,blend(color(0xefede7),color(0x162234),model_->night));brush_->SetColor(blend(color(0x2a312a,.18f),color(0xd0e2f3,.2f),model_->night));renderer_->drawing()->DrawRectangle(rect,brush_.Get(),1);
    text(name,{rect.left+10,rect.top+(rect.bottom-rect.top-14)*.5f,rect.right-20,rect.bottom},11,400,disabled?muted_:ink_);
    float direction=dropdown_.visible&&dropdown_.source.key=="setting-"+group+"-"+key?1-2*dropdown_.progress:1,cy=(rect.top+rect.bottom)*.5f;
    line(rect.right-13,cy-2*direction,rect.right-10,cy+direction,disabled?muted_:ink_);line(rect.right-10,cy+direction,rect.right-7,cy-2*direction,disabled?muted_:ink_);
    Hit hit{"setting-"+group+"-"+key,"dropdown",{{"key",key},{"group",group},{"values",values},{"current",current}},rect,disabled,""};if(panelClipActive_)hit.clip=panelClip_;if(focus_==hit.key&&focusVisible_){brush_->SetColor(accent_);renderer_->drawing()->DrawRectangle({rect.left-5,rect.top-5,rect.right+5,rect.bottom+5},brush_.Get(),2);}if(key=="sortMode")panelNode(".settings-row select",rect);hits_.push_back(std::move(hit));
}
void Ui::toggle(std::string key,std::wstring value,float& y,bool enabled,bool disabled,std::wstring note){
    text(std::move(value),{controlLeft_,y,controlRight_-35,y+20},12,400,disabled?muted_:ink_);
    float height=note.empty()?22.f:21.f+paragraph(std::move(note),controlLeft_,controlRight_-43,y+21,9,4.f/3);
    D2D1_RECT_F rect{controlRight_-19,y+height*.5f-8,controlRight_-3,y+height*.5f+8};
    fill(rect,enabled?(disabled?muted_:accent_):blend(color(0xefede7),color(0x162234),model_->night),2);brush_->SetColor(enabled?(disabled?muted_:accent_):muted_);renderer_->drawing()->DrawRoundedRectangle(D2D1::RoundedRect(rect,2,2),brush_.Get(),1);
    if(enabled){line(rect.left+3,rect.top+8,rect.left+7,rect.top+12,color(0xffffff),1.5f);line(rect.left+7,rect.top+12,rect.left+13,rect.top+4,color(0xffffff),1.5f);}
    Hit hit{"setting-"+key,"set",{{"key",key},{"value",!enabled}}, {controlLeft_,y,controlRight_,y+height},disabled,""};if(panelClipActive_)hit.clip=panelClip_;hits_.push_back(std::move(hit));y+=height+15;
}
void Ui::choice(std::string key,std::wstring value,float& y,const json& values,const json& current,std::string group,std::wstring note,bool disabled){
    float maximum=90;for(auto& item:values)maximum=std::max(maximum,textWidth(wide(item.at("label").get<std::string>()),11)+42.f);
    maximum=std::min(maximum,(controlRight_-controlLeft_)*.58f);
    float height=note.empty()?28.f:33.f;float labelTop=y+(height-(note.empty()?16.f:33.f))*.5f;
    text(std::move(value),{controlLeft_,labelTop,controlRight_-maximum-20,labelTop+16},12,400,disabled?muted_:ink_);
    if(!note.empty())paragraph(std::move(note),controlLeft_,controlRight_-maximum-20,labelTop+21,9,4.f/3);
    selectControl(std::move(key),{controlRight_-maximum,y+(height-28)*.5f,controlRight_,y+(height+28)*.5f},values,current,std::move(group),disabled);y+=height+15;
}
void Ui::slider(std::string key,std::wstring value,float& y,double current,double minimum,double maximum,double step,bool disabled,std::string group){
    bool quality=group=="renderQuality";float length=width_<700?110.f:150.f;
    text(std::move(value),{controlLeft_,y+1,controlRight_-length-(quality?62:20),y+25},quality?11.f:12.f,400,disabled?muted_:ink_);
    D2D1_RECT_F rect{controlRight_-length-(quality?62:0),y,controlRight_-(quality?62:0),y+20};float ratio=float(std::clamp((current-minimum)/(maximum-minimum),0.,1.));
    line(rect.left,rect.top+10,rect.right,rect.top+10,blend(muted_,panelColor_,.7f),3);line(rect.left,rect.top+10,std::lerp(rect.left,rect.right,ratio),rect.top+10,ink_,3);brush_->SetColor(disabled?muted_:ink_);renderer_->drawing()->FillEllipse(D2D1::Ellipse({std::lerp(rect.left,rect.right,ratio),rect.top+10},5,5),brush_.Get());
    if(quality){std::wostringstream n;n<<std::setprecision(3)<<current;text(n.str()+L"%",{controlRight_-44,y,controlRight_,y+20},12,400,muted_);}
    Hit hit{"slider-"+group+"-"+key,"slider",{{"key",key},{"group",group},{"value",current},{"min",minimum},{"max",maximum},{"step",step}},rect,disabled,""};if(panelClipActive_)hit.clip=panelClip_;hits_.push_back(std::move(hit));y+=35;
}
void Ui::panelBackdrop(){
    if(measuring_)return;
    auto drawing=renderer_->drawing();auto size=drawing->GetPixelSize();
    if(!backdrop_||backdrop_->GetPixelSize().width!=size.width||backdrop_->GetPixelSize().height!=size.height){backdrop_.Reset();check(drawing->CreateBitmap(size,nullptr,0,D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE)),&backdrop_),"Panel backdrop bitmap");}
    check(backdrop_->CopyFromRenderTarget(nullptr,drawing,nullptr),"Panel backdrop capture");
    if(!backdropBlur_)check(drawing->CreateEffect(CLSID_D2D1GaussianBlur,&backdropBlur_),"Panel backdrop blur");
    backdropBlur_->SetInput(0,backdrop_.Get());check(backdropBlur_->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION,8.f*scale_),"Original modal blur radius");check(backdropBlur_->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE,D2D1_BORDER_MODE_HARD),"Modal blur border");
    drawing->SetTransform(D2D1::Matrix3x2F::Identity());D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds={0,0,float(size.width),float(size.height)};layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.opacity=panelOpacity_;drawing->PushLayer(layer,nullptr);drawing->DrawImage(backdropBlur_.Get());drawing->PopLayer();drawing->SetTransform(D2D1::Matrix3x2F::Scale(scale_,scale_));fill({0,0,width_,height_},color(0x040c13,.25f*panelOpacity_));
}
void Ui::preparePanelShadow(float width,float height){
    if(measuring_)return;
    auto drawing=renderer_->drawing();D2D1_SIZE_U size{UINT(std::ceil(width)),UINT(std::ceil(height))};
    if(panelShadowMask_&&panelShadowMask_->GetPixelSize().width==size.width&&panelShadowMask_->GetPixelSize().height==size.height)return;
    panelShadowMask_.Reset();check(drawing->CreateBitmap(size,nullptr,0,D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED),96,96),&panelShadowMask_),"Original panel shadow mask");
    Com<ID2D1Image> target;drawing->GetTarget(&target);D2D1_MATRIX_3X2_F transform;drawing->GetTransform(&transform);drawing->SetTarget(panelShadowMask_.Get());drawing->SetTransform(D2D1::Matrix3x2F::Identity());drawing->Clear(color(0,0));brush_->SetColor(color(0xffffff));drawing->FillRectangle({0,0,width,height},brush_.Get());drawing->SetTarget(target.Get());drawing->SetTransform(transform);
    if(!panelShadow_)check(drawing->CreateEffect(CLSID_D2D1Shadow,&panelShadow_),"Original panel shadow effect");panelShadow_->SetInput(0,panelShadowMask_.Get());check(panelShadow_->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION,45.f),"Original panel shadow blur");check(panelShadow_->SetValue(D2D1_SHADOW_PROP_COLOR,D2D1::Vector4F(0,0,0,.13f)),"Original panel shadow ink");
}
void Ui::panel(){
    if(!measuring_){std::string key=visiblePanel_+std::to_string(width_)+":"+std::to_string(height_)+":"+std::to_string(model_->libraryRevision)+model_->preferences.dump()+model_->nativeStatus.dump()+model_->sonyStatus.dump()+model_->ftp.dump()+model_->config.dump()+model_->onlineResults.dump()+model_->onlineSources.dump()+model_->beefweb.dump()+fields().dump()+model_->player.value("queueIds",json::array()).dump();if(key!=panelMeasureKey_){measure([this]{panel();});panelMeasureKey_=std::move(key);}float scrim=width_<=700?12.f:28.f;float h=std::min(height_-scrim*2-(width_<=700?16.f:20.f),panelHeight_);scroll_=std::clamp(scroll_,0.f,std::max(0.f,contentHeight_-h));panelPaintScroll_=scroll_;}
    panelNodes_=json::object();
    if(focus_.empty())focus_="close-panel";
    bool mobile=width_<=700;float scrim=mobile?12.f:28.f,padx=mobile?21.f:35.f,pady=mobile?23.f:30.f;
    float w=std::min(650.f,width_-scrim*2),maxHeight=height_-scrim*2-(mobile?16.f:20.f);
    float h=std::min(maxHeight,panelHeight_>0?panelHeight_:maxHeight),left=(width_-w)*.5f,top=(height_-h)*.5f+panelOffset_;
    panelBounds_={left,top,left+w,top+h};panelClip_={left+1,top+1,left+w-1,top+h-1};controlLeft_=left+padx+1;controlRight_=left+w-padx-1;
    panelBackdrop();
    preparePanelShadow(w,h);
    D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds={0,0,width_,height_};layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.opacity=panelOpacity_;renderer_->drawing()->PushLayer(layer,nullptr);
    if(!measuring_){Com<ID2D1Image> shadowImage;panelShadow_->GetOutput(&shadowImage);D2D1_RECT_F shadowBounds{};check(renderer_->drawing()->GetImageLocalBounds(shadowImage.Get(),&shadowBounds),"Original panel shadow extent");renderer_->drawing()->DrawImage(shadowImage.Get(),D2D1::Point2F(left+shadowBounds.left,top+24+shadowBounds.top));}
    fill(panelBounds_,panelColor_);brush_->SetColor(blend(color(0x2a312a,.18f),color(0xd0e2f3,.2f),model_->night));renderer_->drawing()->DrawRectangle(panelBounds_,brush_.Get(),1);
    hits_.push_back({"panel-body","none",{},panelBounds_,false,""});
    renderer_->drawing()->PushAxisAlignedClip(panelClip_,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);panelClipActive_=true;
    float y=top+pady+1-scroll_;
    std::string id=visiblePanel_;std::wstring eyebrow=id=="settings"?L"SYSTEM SETTINGS":id=="library"?L"MUSIC LIBRARY":id=="search"?L"FIND MUSIC":id=="online"?L"ONLINE LIBRARY":id=="controller-help"?L"CONTROLLER GUIDE":L"RHINE MUSIC";
    std::wstring name=id=="settings"?L"播放与画质":id=="library"?L"本地音乐库":id=="search"?L"搜索专辑与歌曲":id=="online"?L"在线曲库":id=="controller-help"?L"手柄操作":id=="genres"?L"流派归并规则":id=="beefweb"?L"Beefweb 播放列表":id=="archive-index"?L"ARCHIVE INDEX":id=="archive-saved"?L"SAVED ARCHIVES":L"播放队列";
    panelNode(".panel-heading",{controlLeft_,y,controlRight_,y+(mobile?57.f:59.f)});
    text(eyebrow,{controlLeft_,y+8,controlRight_-32,y+22},9,400,muted_,false,false,2);
    text(name,{controlLeft_,y+29,controlRight_-32,y+64},mobile?21.f:23.f,500,ink_);
    Hit close{"close-panel","close-panel",{}, {controlRight_-textWidth(L"×",29),y,controlRight_,y+29},false,""};close.clip=panelClip_;hits_.push_back(close);text(L"×",close.rect,29,400,muted_,false,false,0,1);panelNode(".panel-heading > button",close.rect);if(focus_==close.key&&focusVisible_){brush_->SetColor(accent_);renderer_->drawing()->DrawRectangle({close.rect.left-5,close.rect.top-5,close.rect.right+5,close.rect.bottom+5},brush_.Get(),2);}y+=mobile?80:82;
    float bodyTop=y;
    if(id=="settings")settings(controlLeft_-40,controlRight_,y);
    else if(id=="library")library(controlLeft_-40,controlRight_,y);
    else if(id=="search")search(controlLeft_-40,controlRight_,y);
    else if(id=="online")online(controlLeft_-40,controlRight_,y);
    else if(id=="archive-index"||id=="archive-saved")archiveSearch(controlLeft_-40,controlRight_,y);
    else if(id=="controller-help")controllerHelp(controlLeft_,controlRight_,y);
    else if(id=="beefweb")beefweb(controlLeft_-40,controlRight_,y);
    else if(id=="genres"){input("genre-json",L"本地流派规则 · 只修改索引分类",{controlLeft_,y+25,controlRight_,y+std::max(180.f,maxHeight-190)},wide(model_->genres.dump(2)),true);y+=std::max(220.f,maxHeight-150);panelButton("save-genres",L"保存并应用",{controlLeft_,y,controlLeft_+125,y+39},"save-genres",{},true);y+=39;}
    else {for(auto& trackId:model_->player.value("queueIds",json::array()))for(auto& album:model_->library.value("albums",json::array()))for(auto& track:album.value("tracks",json::array()))if(track.value("id","")==trackId.get<std::string>()){panelButton("queue-"+trackId.get<std::string>(),wide(track.value("title","")),{controlLeft_,y,controlRight_,y+34},"play",{{"albumId",album.value("id","")},{"trackId",trackId}});y+=39;}}
    panelNode("#panel-body",{controlLeft_,bodyTop,controlRight_,y});if(id=="library"||id=="online")y+=9;contentHeight_=y+scroll_-top+pady+1;panelHeight_=contentHeight_;scroll_=std::clamp(scroll_,0.f,std::max(0.f,contentHeight_-h));
    panelClipActive_=false;renderer_->drawing()->PopAxisAlignedClip();
    if(!measuring_)scrollbar(ScrollArea::panel,{left+w-13,top+5,left+w+6,top+h-5},h,contentHeight_);
    renderer_->drawing()->PopLayer();
    if(focus_.empty())focus_="close-panel";if(!measuring_)drawDropdown();
}
void Ui::setDropdownOpen(bool open){
    if(!model_)return;float duration=dropdown_.open?.18f:.14f;
    dropdown_.progress=std::lerp(dropdown_.from,dropdown_.open?1.f:0.f,float(std::clamp((model_->now-dropdown_.changed)/duration,0.,1.)));
    dropdown_.from=dropdown_.progress;dropdown_.changed=model_->now;dropdown_.open=open;
    dropdown_.visible=open||dropdown_.progress>.0001f;
    if(model_->preferences.value("reduced",false)){dropdown_.progress=open?1.f:0.f;dropdown_.from=dropdown_.progress;dropdown_.visible=open;}
}
void Ui::drawDropdown(){
    if(!dropdown_.visible)return;
    dropdown_.progress=model_->preferences.value("reduced",false)?(dropdown_.open?1.f:0.f):std::lerp(dropdown_.from,dropdown_.open?1.f:0.f,float(std::clamp((model_->now-dropdown_.changed)/(dropdown_.open?.18:.14),0.,1.)));
    if(!dropdown_.open&&dropdown_.progress<=.0001f){dropdown_.visible=false;return;}
    for(auto& [key,edit]:inputs_)if(edit.handle)ShowWindow(edit.handle,SW_HIDE);
    auto& source=dropdown_.source;auto& values=source.args.at("values");int count=std::min(8,int(values.size()));dropdown_.first=std::clamp(dropdown_.first,0,std::max(0,int(values.size())-count));if(dropdown_.selected<dropdown_.first)dropdown_.first=dropdown_.selected;if(dropdown_.selected>=dropdown_.first+count)dropdown_.first=dropdown_.selected-count+1;
    float height=count*29.f+4,top=source.rect.bottom+2;bool above=top+height>panelClip_.bottom;if(above)top=std::max(panelClip_.top,source.rect.top-height-2);D2D1_RECT_F rect{source.rect.left,top,source.rect.right,top+height};dropdown_.bounds=rect;
    dropdown_.highlight=std::lerp(dropdown_.highlightFrom,dropdown_.highlightTarget,float(std::clamp((model_->now-dropdown_.highlightChanged)/.12,0.,1.)));
    float target=float(dropdown_.selected-dropdown_.first)*29;if(target!=dropdown_.highlightTarget){dropdown_.highlightFrom=dropdown_.highlight;dropdown_.highlightTarget=target;dropdown_.highlightChanged=model_->now;}
    if(model_->preferences.value("reduced",false))dropdown_.highlight=target;
    D2D1_RECT_F reveal=rect;if(above)reveal.top=rect.bottom-height*dropdown_.progress;else reveal.bottom=rect.top+height*dropdown_.progress;
    hits_.clear();hits_.push_back({"dropdown-dismiss","dropdown-dismiss",{},{0,0,width_,height_},!dropdown_.open,""});
    auto drawing=renderer_->drawing();D2D1_LAYER_PARAMETERS1 layer{};layer.contentBounds=reveal;layer.maskTransform=D2D1::Matrix3x2F::Identity();layer.opacity=dropdown_.progress;drawing->PushLayer(layer,nullptr);drawing->PushAxisAlignedClip(reveal,D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    fill(rect,panelColor_,3);brush_->SetColor(blend(muted_,panelColor_,.42f));drawing->DrawRoundedRectangle(D2D1::RoundedRect(rect,3,3),brush_.Get(),1);
    D2D1_RECT_F selection{rect.left+2,top+2+dropdown_.highlight,rect.right-2,top+31+dropdown_.highlight};fill(selection,ink_,2);
    for(int i=0;i<count;++i){int index=dropdown_.first+i;D2D1_RECT_F row{rect.left+2,top+2+i*29,rect.right-2,top+2+(i+1)*29};float overlap=std::max(0.f,std::min(row.bottom,selection.bottom)-std::max(row.top,selection.top))/29;
        text(wide(values[index].at("label").get<std::string>()),{row.left+8,row.top+5,row.right-5,row.bottom},11,400,blend(ink_,panelColor_,overlap));auto args=source.args;args["value"]=values[index].at("value");Hit hit{"dropdown-option-"+std::to_string(index),"dropdown-select",args,row,!dropdown_.open,""};hit.clip=reveal;hits_.push_back(std::move(hit));}
    drawing->PopAxisAlignedClip();drawing->PopLayer();
}
