// Measure changed content before painting. A one-pixel offscreen target keeps
// the exact DirectWrite layout path, including the original title reels. It is
// reused and invoked only when content or viewport dimensions change.
void Ui::measure(const std::function<void()>& layout){
    auto drawing=renderer_->drawing();
    if(!measurementTarget_)check(drawing->CreateBitmap({1,1},nullptr,0,D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET,D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_PREMULTIPLIED)),&measurementTarget_),"UI layout measurement target");
    Com<ID2D1Image> target;drawing->GetTarget(&target);auto hitCount=hits_.size(),badgeCount=badgeEvidence_.size();
    drawing->SetTarget(measurementTarget_.Get());measuring_=true;
    try{layout();}catch(...){measuring_=false;drawing->SetTarget(target.Get());hits_.resize(hitCount);throw;}
    measuring_=false;drawing->SetTarget(target.Get());hits_.resize(hitCount);
    while(badgeEvidence_.size()>badgeCount)badgeEvidence_.erase(badgeEvidence_.end()-1);
}
void Ui::playPauseGlyph(const std::string& key,D2D1_RECT_F rect,D2D1_COLOR_F paint){
    const float t=playPauseAmount_;
    // Two halves of the triangle unfold into the two pause bars. All vertices
    // share one linear clock, so reversing during a transition stays continuous.
    const std::array<std::array<D2D1_POINT_2F,4>,2> play{{{{{8,4.5f},{14,8.25f},{14,15.75f},{8,19.5f}}},{{{14,8.25f},{20,12},{20,12},{14,15.75f}}}}};
    const std::array<std::array<D2D1_POINT_2F,4>,2> pause{{{{{7,5},{10,5},{10,19},{7,19}}},{{{14,5},{17,5},{17,19},{14,19}}}}};
    auto drawing=renderer_->drawing();brush_->SetColor(paint);
    if(!playPauseShape_||playPauseShapeAmount_!=t){Com<ID2D1Factory> factory;drawing->GetFactory(&factory);std::array<Com<ID2D1PathGeometry>,2> parts;playPauseVertices_=json::array();
        for(size_t part=0;part<2;++part){check(factory->CreatePathGeometry(&parts[part]),"Play/pause morph geometry");Com<ID2D1GeometrySink> sink;check(parts[part]->Open(&sink),"Play/pause morph vertices");json points=json::array();
            for(size_t i=0;i<4;++i){auto p=D2D1::Point2F(std::lerp(play[part][i].x,pause[part][i].x,t),std::lerp(play[part][i].y,pause[part][i].y,t));points.push_back({p.x,p.y});if(i==0)sink->BeginFigure(p,D2D1_FIGURE_BEGIN_FILLED);else sink->AddLine(p);}
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);check(sink->Close(),"Play/pause morph shape");playPauseVertices_.push_back(points);
        }
        ID2D1Geometry* members[]={parts[0].Get(),parts[1].Get()};playPauseShape_.Reset();check(factory->CreateGeometryGroup(D2D1_FILL_MODE_WINDING,members,2,&playPauseShape_),"Seamless play/pause morph");playPauseShapeAmount_=t;
    }
    D2D1_MATRIX_3X2_F previous;drawing->GetTransform(&previous);drawing->SetTransform(D2D1::Matrix3x2F::Scale((rect.right-rect.left)/24,(rect.bottom-rect.top)/24)*D2D1::Matrix3x2F::Translation(rect.left,rect.top)*previous);drawing->FillGeometry(playPauseShape_.Get(),brush_.Get());drawing->SetTransform(previous);
    playPauseEvidence_[key]={{"amount",t},{"target",playPauseTarget_?"pause":"play"},{"vertices",playPauseVertices_},{"durationMs",220},{"timing","linear"},{"bounds",{rect.left,rect.top,rect.right,rect.bottom}}};
}
float Ui::scrollMaximum(ScrollArea area)const{
    return area==ScrollArea::panel?std::max(0.f,contentHeight_-(panelBounds_.bottom-panelBounds_.top)):area==ScrollArea::detail?std::max(0.f,detailContentHeight_-(detailPaneBottom_-detailPaneTop_)):0;
}
void Ui::scrollTo(ScrollArea area,float value,bool manual){
    const float next=std::clamp(value,0.f,scrollMaximum(area));
    if(area==ScrollArea::panel)scroll_=next;
    else if(area==ScrollArea::detail){detailScroll_=next;if(manual){detailScrollMotion_.value=next;detailScrollMotion_.velocity=0;cancelLocation();emit_("cancel-location",{});}}
}
void Ui::scrollbar(ScrollArea area,D2D1_RECT_F rail,float viewport,float content){
    auto& bar=area==ScrollArea::panel?panelScrollbar_:detailScrollbar_;bar={};bar.rail=rail;bar.viewport=viewport;bar.maximum=std::max(0.f,content-viewport);
    float height=rail.bottom-rail.top;if(bar.maximum<.01f||height<1)return;bar.visible=true;
    float thumb=std::clamp(height*viewport/std::max(1.f,content),std::min(24.f,height),height);bar.travel=height-thumb;
    float value=area==ScrollArea::panel?scroll_:detailScroll_,offset=bar.travel*value/bar.maximum;float center=(rail.left+rail.right)*.5f;
    bar.thumb={rail.left,rail.top+offset,rail.right,rail.top+offset+thumb};
    bool hover=contains(rail,pointerX_,pointerY_)||scrollbarDrag_==area;
    auto paint=muted_;paint.a=hover?.8f:.45f;fill({center-1.5f,bar.thumb.top,center+1.5f,bar.thumb.bottom},paint,1.5f);
    std::string key=area==ScrollArea::panel?"panel-scrollbar":"detail-scrollbar";
    hits_.push_back({key,"scrollbar",{{"area",area==ScrollArea::panel?"panel":"detail"}},rail,false,""});layoutNode(key,bar.thumb);
}
std::wstring Ui::searchNormalized(const std::wstring& source){
    auto found=searchCache_.find(source);if(found!=searchCache_.end())return found->second;
    // Fold both input and display text with the same official OpenCC character
    // table, including Taiwan/Hong Kong variants. Original tags are untouched.
    static const auto characters=[](){std::ifstream file(executableDirectory()/L"runtime/vendor/opencc/search-characters.json");if(!file)throw std::runtime_error("Offline Chinese search dictionary missing");json data;file>>data;std::unordered_map<std::wstring,std::wstring> result;for(auto it=data.begin();it!=data.end();++it)result.emplace(wide(it.key()),wide(it.value().get<std::string>()));return result;}();
    std::wstring value;for(size_t i=0;i<source.size();){size_t length=source[i]>=0xd800&&source[i]<=0xdbff&&i+1<source.size()&&source[i+1]>=0xdc00&&source[i+1]<=0xdfff?2:1;auto character=source.substr(i,length);auto mapped=characters.find(character);value+=mapped==characters.end()?character:mapped->second;i+=length;}
    for(auto& c:value){if(c>=0xff01&&c<=0xff5e)c-=0xfee0;if(c==0x3000)c=L' ';c=std::towlower(c);}
    if(searchCache_.size()>=8192)searchCache_.clear();searchCache_[source]=value;return value;
}
