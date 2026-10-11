void Ui::archiveHeader(){
    bool portrait=width_/height_<1.05;float inset=portrait?20:40;
    const float brandY=portrait?18.f:27.f,brandScale=portrait?.7f:.82f;
    text(L"RHINE LAB",{inset,brandY,inset+300,brandY+48*brandScale},44*brandScale,700);
    text(L"SYNTHESIZE INFORMATION",{inset,brandY+48*brandScale,inset+300,brandY+71*brandScale},19*brandScale,600);
    const std::array<float,8> positions{2,28,55,81,103,129,154,166};const std::wstring analysis=L"ANALYSIS";
    for(size_t i=0;i<analysis.size();++i)text(analysis.substr(i,1),{inset+positions[i]*brandScale,brandY+71*brandScale,inset+(positions[i]+40)*brandScale,brandY+111*brandScale},35*brandScale,400);
    text(L"OS",{inset+216*brandScale,brandY+71*brandScale,inset+282*brandScale,brandY+111*brandScale},35*brandScale,700);
    float x=portrait?inset:width_-520,y=portrait?105.f:40.f;
    button("archive-index",L"⌕  ARCHIVE INDEX  /",{x,y,x+180,y+36},"panel",{{"panel","archive-index"}});
    button("archive-saved",L"＋ SAVED "+number(int(model_->preferences.value("archiveSaved",json::array()).size())),{x+186,y,x+313,y+36},"panel",{{"panel","archive-saved"}});
    button("archive-settings",L"◷",{x+319,y,x+355,y+36},"panel",{{"panel","settings"}});
    button("archive-music",L"音乐终端 ↗",{x+361,y,x+487,y+36},"original",{{"enabled",false}});
}
void Ui::beefweb(float left,float right,float& y){
    text(L"BE EF WEB / PLAYLISTS",{left+40,y,right,y+28},10,400,muted_);y+=42;
    const auto& lists=model_->beefweb.contains("playlists")&&model_->beefweb["playlists"].is_array()?model_->beefweb["playlists"]:json::array();
    for(auto& item:lists){text(wide(item.value("title",item.value("name",std::string("播放列表")))),{left+40,y,right,y+40},17,500);y+=48;text(wide(item.value("id",std::string()))+L"  /  "+std::to_wstring(item.value("itemCount",0))+L" TRACKS",{left+40,y,right,y+24},10,400,muted_);y+=43;}
    if(lists.empty())y+=text(wide(model_->beefweb.dump(2)),{left+40,y,right,y+20000},11,400,ink_,true)+32;
}
void Ui::archiveBrowse(){
    const auto& records=arrayField(model_->library,"albums");if(records.empty())return;auto& record=records[std::clamp(model_->selected,0,int(records.size())-1)];bool portrait=width_/height_<1.05;
    float left=portrait?24:width_*.623f,right=width_-std::max(24.f,width_*.022f),y=portrait?height_*.54f:height_*.30f;
    text(L"FILE "+wide(record.value("id","")),{left,y,right,y+28},10);y+=42;
    title(browseTitle_,record.value("en",""),{left,y,right,height_-180},portrait?30.f:44.f);y+=browseTitle_.height+18;
    text(wide(record.value("title","")),{left,y,right,y+35},21,500);y+=48;
    text(wide(record.value("department",""))+L"  /  "+wide(record.value("category","")),{left,y,right,y+28},10,400,muted_);y+=40;
    button("archive-read",L"READ ARCHIVE / 读取档案  ↗",{left,y,right,y+48},"open");
    float nav=height_-115;button("archive-previous",L"↑",{42,nav,91,nav+43},"navigate",{{"row",-1}});button("archive-next",L"↓",{99,nav,148,nav+43},"navigate",{{"row",1}});button("archive-lane-left",L"←",{left,nav,left+48,nav+43},"navigate",{{"lane",-1}});button("archive-lane-right",L"→",{right-48,nav,right,nav+43},"navigate",{{"lane",1}});
    text(L"NO."+number(model_->selected+1,3)+L" / "+number(int(records.size()),3),{42,height_-43,360,height_-18},9,400,muted_);
}
void Ui::archiveDetail(){
    if(!measuring_){std::string key="archive:"+std::to_string(width_)+":"+std::to_string(height_)+":"+std::to_string(model_->selected)+model_->preferences.value("archiveTab",std::string("overview"))+model_->archiveLog.dump()+model_->preferences.value("archiveSaved",json::array()).dump();if(key!=detailMeasureKey_){measure([this]{archiveDetail();});detailMeasureKey_=std::move(key);}scrollTo(ScrollArea::detail,detailScroll_,false);detailPaintScroll_=detailScroll_;}
    redactions_.clear();
    backControl("archive-back",L"返回档案阵列",42,112);const auto& records=arrayField(model_->library,"albums");if(records.empty())return;auto& record=records[std::clamp(model_->selected,0,int(records.size())-1)];const std::string identity=record.value("id","");bool portrait=width_/height_<1.05;
    float left=(portrait?24:width_*.52f)+model_->detailOffset,right=(portrait?width_-24:width_*.955f)+model_->detailOffset,top=portrait?height_*.5f:170,bottom=height_-52;
    if(detailIdentity_!=identity){detailIdentity_=identity;detailScroll_=0;decryptionVisibleSince_=model_->now;}
    detailPaneTop_=top;detailPaneBottom_=bottom;detailHitClip_={left,top,right,bottom};detailClipActive_=true;renderer_->drawing()->PushAxisAlignedClip({left,top,right,bottom},D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);float y=top-detailScroll_;
    text(L"FILE "+wide(identity),{left,y,right,y+25},9);text(wide(record.value("clearance","")),{right-175,y,right,y+25},9,400,muted_);y+=38;
    redactText_=true;title(detailTitle_,record.value("en",""),{left,y,right,y+400},std::clamp(width_*.0245f,24.f,39.f));y+=detailTitle_.height+12;
    text(wide(record.value("title",""))+L"  /  "+wide(record.value("category","")),{left,y,right,y+40},16);y+=54;line(left,y,right,y,ink_);y+=22;
    const std::array<std::pair<const wchar_t*,std::string>,4> facts{{{L"DEPARTMENT / 科室",record.value("department","")},{L"COLLECTION / 编目范围",record.value("date","")},{L"RELATED / 相关人物",record.value("lead","")},{L"STATUS / 状态",record.value("clearance","")=="RESTRICTED"?"目录访问":"已归档 · 可读取"}}};
    for(auto& fact:facts){text(fact.first,{left,y,left+(right-left)*.42f,y+24},8,400,muted_);y+=std::max(28.f,text(wide(fact.second),{left+(right-left)*.44f,y,right,y+100},11,400,ink_,true))+14;}
    redactText_=false;std::string tab=model_->preferences.value("archiveTab",std::string("overview"));float tabWidth=(right-left)/3;
    const std::array<std::pair<const char*,const wchar_t*>,3> tabs{{{"overview",L"01  概述"},{"notes",L"02  研究记录"},{"history",L"03  访问日志"}}};
    for(int i=0;i<3;++i)button(std::string("archive-tab-")+tabs[i].first,tabs[i].second,{left+i*tabWidth,y,left+(i+1)*tabWidth-4,y+42},"archive-tab",{{"tab",tabs[i].first}},tab==tabs[i].first);y+=61;
    redactText_=true;if(tab=="overview"){text(L"ABSTRACT / 摘要",{left,y,right,y+24},8,400,muted_);y+=33;y+=text(wide(record.value("abstract","")),{left,y,right,y+1200},13,400,ink_,true)+30;}
    else if(tab=="notes"){text(L"RESEARCH NOTES / 研究记录",{left,y,right,y+24},8,400,muted_);y+=35;int index=0;for(auto& finding:arrayField(record,"findings")){text(number(++index),{left,y,left+24,y+25},8,400,muted_);y+=text(wide(finding.get<std::string>()),{left+35,y,right,y+400},13,400,ink_,true)+27;}}
    else{ text(L"ACCESS LOG / 本次访问",{left,y,right,y+24},8,400,muted_);y+=34;int count=0;for(auto& entry:model_->archiveLog)if(entry.value("id","")==identity&&count++<4){text(wide(entry.value("time",""))+L"    JOYCE MOORE    READ AUTHORIZED",{left,y,right,y+32},9);y+=41;}}
    redactText_=false;bool saved=false;for(auto& id:arrayField(model_->preferences,"archiveSaved"))if(id==identity)saved=true;
    button("archive-bookmark",saved?L"− REMOVE FROM SAVED / 已收藏":L"＋ SAVE ARCHIVE / 收藏档案",{left,y,right-118,y+47},"archive-bookmark",{{"id",identity}},saved);
    button("archive-export",L"EXPORT ↓",{right-110,y,right,y+47},"archive-export",{{"id",identity}});y+=65;
    button("archive-source",L"设定参考 ↗",{left,y,left+145,y+37},"link",{{"url",record.value("source","")}});y+=54;
    button("archive-viewer",L"360° 查看档案模型 ↗",{left,y,right,y+44},"viewer-open");y+=65;
    drawRedactions();detailContentHeight_=y+detailScroll_-top;renderer_->drawing()->PopAxisAlignedClip();detailScroll_=std::clamp(detailScroll_,0.f,std::max(0.f,detailContentHeight_-(bottom-top)));
    detailClipActive_=false;if(!measuring_)scrollbar(ScrollArea::detail,{right+1,top,right+18,bottom},bottom-top,detailContentHeight_);text(L"NO."+number(model_->selected+1,3),{42,height_-103,340,height_-70},13);
}
void Ui::archiveSearch(float left,float right,float& y){
    input("archive-search",L"档案编号、名称或科室",{left+40,y+25,right,y+68},L"");y+=101;json categories=json::array({{{"value",""},{"label","全部档案"}}});for(auto& genre:arrayField(model_->library,"genres"))categories.push_back({{"value",genre.value("id","")},{"label",genre.value("name","")}});choice("archiveCategory",L"科室分类",y,categories,model_->preferences.value("archiveCategory",json("")),"view");
    std::wstring query=field("archive-search");std::transform(query.begin(),query.end(),query.begin(),towlower);int count=0;
    const auto& records=arrayField(model_->library,"albums");for(size_t i=0;i<records.size();++i){const auto& record=records[i];auto category=model_->preferences.value("archiveCategory",std::string(""));if(!category.empty()&&record.value("category","")!=category)continue;bool saved=false;for(auto& id:arrayField(model_->preferences,"archiveSaved"))if(id==record.value("id",""))saved=true;if(model_->panel=="archive-saved"&&!saved)continue;std::wstring searchable=wide(record.value("id","")+" "+record.value("title","")+" "+record.value("en","")+" "+record.value("department","")+" "+record.value("lead",""));std::transform(searchable.begin(),searchable.end(),searchable.begin(),towlower);if(searchable.find(query)==std::wstring::npos)continue;
        ++count;button("archive-result-"+std::to_string(i),wide(record.value("id",""))+L"   "+wide(record.value("title",""))+(saved?L"   ＋":L""),{left+40,y,right,y+49},"archive-select",{{"index",i}});y+=57;text(wide(record.value("en",""))+L" / "+wide(record.value("department","")),{left+40,y,right,y+45},9,400,muted_,true);y+=57;}
    text(std::to_wstring(count)+L" RECORDS FOUND",{left+40,y,right,y+29},9,400,muted_);y+=43;
    if(!count){text(model_->panel=="archive-saved"?L"尚无收藏档案":L"没有匹配的档案",{left+40,y,right,y+34},17,500);y+=50;button("archive-reset-search",L"查看全部档案 →",{left+40,y,right,y+44},"archive-reset-search");y+=60;}
}
