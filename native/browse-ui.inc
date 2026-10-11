void Ui::browse(){
    const auto& albums=arrayField(model_->library,"albums");
    if(albums.empty()){text(model_->shelf=="online"?L"你的在线音乐架":L"让音乐进入这座档案馆。",{width_*.55f,height_*.37f,width_-42,height_*.55f},32,600,ink_,true);button("empty-library",model_->shelf=="online"?L"寻找在线音乐 ↗":L"设置音乐文件夹 ↗",{width_*.55f,height_*.55f,width_-42,height_*.55f+54},"panel",{{"panel",model_->shelf=="online"?"online":"library"}});return;}
    const bool mobile=width_<=700,narrow=width_<=1100,large=width_>=1700,shortView=height_<=650&&!mobile;
    const int index=std::clamp(model_->selected,0,int(albums.size())-1);const auto& album=albums[index];
    auto files=arrayField(model_->library,"columnFiles"),columns=arrayField(model_->library,"columns");int lane=0,ordinal=0,count=1;
    for(size_t i=0;i<files.size();++i){auto it=std::find(files[i].begin(),files[i].end(),json(index));if(it!=files[i].end()){lane=int(i);ordinal=int(it-files[i].begin());count=int(files[i].size());break;}}
    std::wstring genre=lane<int(columns.size())?label(columns[lane]):L"未分类";
    const float navLeft=mobile?23.f:narrow?27.f:large?62.f:42.f,navRight=mobile?width_-23:width_*.938f,gap=mobile?12.f:narrow?20.f:40.f,counterWidth=mobile?74.f:narrow?145.f:210.f;
    const float navHeight=mobile?62.f:narrow?70.3f:large?96.7f:78.3f;
    float footerTop=height_;if(playbackVisible_)footerTop=playbackLayout(width_,height_).top;if(selectedCd())footerTop=std::min(footerTop,cdEjectRect().top);
    const bool footerOccupied=playbackVisible_||selectedCd();
    const float occupiedHeight=std::max(navHeight,(mobile?22.f:29.f)+(mobile?34.f:narrow?42.f:large?65.f:50.f)*1.3f);
    float navTop=height_-(mobile?65.f:narrow?57.f:large?90.f:60.f)-navHeight;
    if(footerOccupied)navTop=std::min(navTop,footerTop-(mobile?16.f:36.f)-occupiedHeight);
    const float col=(navRight-navLeft-counterWidth-gap*2)/(narrow?2.f:2.1f),stepCenter=navLeft+counterWidth+gap+col*.5f,genreLeft=navLeft+counterWidth+gap+col+gap,genreCenter=(genreLeft+navRight)*.5f;
    float left=portrait_?std::clamp(width_*.04f,23.f,42.f):width_*.68633333f,right=portrait_?width_-std::clamp(width_*.04f,23.f,42.f)-12:width_-std::max(24.f,width_*.022f);
    const float calloutBottom=height_-navTop+28,calloutHeight=std::min(std::max(210.f,height_*.33333f),height_-calloutBottom-150);
    float top=portrait_?height_-calloutBottom-calloutHeight+10:shortView?125.f:large?height_*.32f:narrow?std::clamp(height_*.28f,132.f,240.f):std::clamp(height_*.30f,155.f,330.f);
    layoutNode("browse-caption",{left,top,right,top+calloutHeight});
    const float eyebrow=mobile?7.f:large?11.f:9.f,ruleFont=mobile?8.f:large?12.f:10.f;
    const auto& tracks=arrayField(album,"tracks");std::wstring formats;std::vector<std::string> known;double total=0;
    for(const auto& track:tracks){total+=track.value("duration",0.);auto f=track.value("format",std::string());if(!f.empty()&&std::find(known.begin(),known.end(),f)==known.end()){known.push_back(f);if(!formats.empty())formats+=L" / ";formats+=wide(f);}}
    const std::wstring prefix=L"MUSIC ARCHIVE ";const float prefixWidth=textWidth(prefix,eyebrow)+float(prefix.size())*1.5f,slashX=left+prefixWidth+4;
    text(prefix,{left,top,right,top+eyebrow*1.7f},eyebrow,400,ink_,false,false,1.5f,1.7f);text(L"／",{slashX,top,slashX+eyebrow+2,top+eyebrow*1.7f},eyebrow,400,ink_,false,false,1.5f,1.7f);
    const float genreX=slashX+eyebrow+1.5f+8+textWidth(L" ",eyebrow)+1.5f;
    movingText("selection-genre",genre,{genreX,top,right,top+eyebrow*1.7f},eyebrow,400,ink_,1.5f,1.7f);
    float y=top+eyebrow*1.7f,ruleHeight=mobile?29.f:shortView?28.f:41.f,rulePad=mobile?9.f:shortView?7.f:15.f;
    text(L"ALBUM ",{left,y+rulePad,right,y+ruleHeight},ruleFont,400,ink_,false,false,1);
    movingText("selection-code",number(index+1,3),{left+textWidth(L"ALBUM ",ruleFont)+5,y+rulePad,right,y+ruleHeight},ruleFont,400,ink_,1);
    const float formatWidth=textWidth(formats,9)+float(formats.size())*1;
    movingText("selection-format",formats,{right-formatWidth,y+rulePad,right,y+ruleHeight},9,400,muted_,1);
    line(left,y+ruleHeight-1,right,y+ruleHeight-1,ink_);y+=ruleHeight+(mobile?11.f:shortView?10.f:17.f);
    float size=mobile?25.f:narrow?28.f:std::clamp(width_*.027f,26.f,44.f),titleTop=y;
    const float badgeHeight=album.value("hiRes",false)?std::clamp(width_*.035f,40.f,54.f):0;
    float titleRight=albumBadges(album,left,right,y,size);
    const float artistFont=mobile?12.f:narrow?13.f:large?19.f:15.f,artistGap=mobile||shortView?10.f:21.f,metaFont=mobile?9.f:large?12.f:10.f,openGap=mobile?13.f:shortView?12.f:22.f,openHeight=mobile?42.f:shortView?41.f:49.f;
    const float remaining=(portrait_?top+calloutHeight-20:height_-calloutBottom)-y-(mobile?9.f:13.f)-artistFont*1.6f-artistGap-metaFont*1.3f-openGap-openHeight;
    title(browseTitle_,album.value("title",""),{left,y,titleRight,y+std::max(28.f,remaining)},size);y+=std::max(browseTitle_.height,badgeHeight)+(mobile?9.f:13.f);
    movingText("selection-artist",wide(album.value("artist","")),{left,y,right,y+120},artistFont,600,ink_,0,1.6f,true);y+=textHeight(wide(album.value("artist","")),artistFont,right-left,1.6f)+artistGap;
    const auto meta=(album.contains("year")?label(album["year"]):L"年份未提供")+L"  /  "+std::to_wstring(tracks.size())+L" 首曲目"+(tracks.empty()?L"":L"  /  "+duration(total));
    movingText("selection-meta",meta,{left,y,right,y+80},metaFont,500,muted_,.5f,1.3f,true);y+=textHeight(meta,metaFont,right-left,1.3f)+openGap;
    const auto openRect=D2D1::RectF(left,y,right,y+openHeight);flatButton("open",L"打开专辑",openRect,"open",{},16,ink_);text(L"↗",{right-24,y+(shortView||mobile?8:10),right,y+openHeight},24,400,ink_);line(left,y+openHeight-1,right,y+openHeight-1,blend(muted_,panelColor_,.7f));
    layoutNode("#selection-title",{left,titleTop,titleRight,titleTop+std::max(browseTitle_.height,badgeHeight)});layoutNode(".open-album",openRect);
    text(L"ALBUM / SELECT",{navLeft,navTop+8,navLeft+counterWidth,navTop+25},mobile?6.f:large?11.f:9.f,400,ink_,false,false,mobile?1.f:1.5f);
    const float numberFont=mobile?34.f:narrow?42.f:large?65.f:50.f,numberY=navTop+(mobile?22.f:29.f);
    movingText("selection-number",number(ordinal+1),{navLeft,numberY,navLeft+counterWidth,numberY+numberFont*1.3f},numberFont,300,ink_,-3,1);
    float numRight=navLeft+textWidth(number(ordinal+1),numberFont,300)-6+8;const float totalSize=mobile?9.f:12.f,totalY=numberY+numberFont-totalSize;
    text(L"/",{numRight,totalY,numRight+14,totalY+20},totalSize,300,muted_);movingText("selection-total",number(count),{numRight+(mobile?8:16),totalY,navLeft+counterWidth,totalY+20},totalSize,300,muted_,mobile?0.f:1.f,1.3f);
    const float arrow=mobile?21.f:25.f,arrowWidth=mobile?21.f:25.f,stepGap=mobile?14.f:21.f,tickHeight=mobile?21.f:30.f,centerY=navTop+navHeight*.5f;
    drawRuler(stepCenter,centerY-tickHeight*.5f);
    flatButton("previous-album",L"↑",{rulerRect_.left-stepGap-arrowWidth,centerY-14,rulerRect_.left-stepGap,centerY+14},"navigate",{{"row",-1}},arrow,ink_);
    flatButton("next-album",L"↓",{rulerRect_.right+stepGap,centerY-14,rulerRect_.right+stepGap+arrowWidth,centerY+14},"navigate",{{"row",1}},arrow,ink_);
    const float genreMin=mobile?64.f:118.f,genreGap=mobile?10.f:narrow?15.f:28.f,genreArrow=mobile?20.f:25.f,groupWidth=genreMin+genreGap*2+genreArrow*2,groupLeft=genreCenter-groupWidth*.5f;
    flatButton("previous-lane",L"←",{groupLeft,centerY-15,groupLeft+genreArrow,centerY+15},"navigate",{{"lane",-1}},genreArrow,ink_);
    flatButton("next-lane",L"→",{groupLeft+groupWidth-genreArrow,centerY-15,groupLeft+groupWidth,centerY+15},"navigate",{{"lane",1}},genreArrow,ink_);
    const auto mode=model_->preferences.value("sortMode",std::string("genre"));const std::wstring code=mode=="artist"?L"ARTIST":mode=="album"?L"ALBUMS":L"GENRE",columnLabel=mode=="artist"?L"歌手":mode=="album"?L"分组":L"流派";
    const float genreTextX=groupLeft+genreArrow+genreGap,genreFont=mobile?11.f:14.f,positionSize=mobile?6.f:8.f;
    movingText("genre-position",code+L" "+number(lane+1)+L" / "+number(int(columns.size())),{genreTextX,centerY-(mobile?17.5f:18.5f),genreTextX+genreMin,centerY},positionSize,400,muted_,mobile?.8f:1.3f,1.3f);
    movingText("genre-name",genre,{genreTextX,centerY+(mobile?-1.f:0.f),genreTextX+genreMin,centerY+24},genreFont,400,ink_,1,1.3f);
    hits_.push_back({"genre-name","panel",{{"panel","genres"}},{genreTextX,centerY,genreTextX+genreMin,centerY+24},false,""});
    layoutNode(".music-navigation",{navLeft,navTop,navRight,navTop+navHeight});
    layoutNode("music-navigation-occupied",{navLeft,navTop,navRight,navTop+occupiedHeight});
    if(!mobile){float hintX=narrow?width_*.30f:width_*.5f-136,hintY=height_-(narrow?33.f:large?57.f:36.f)-10;if(footerOccupied)hintY=std::min(hintY,footerTop-28);auto hintRect=D2D1::RectF(hintX,hintY,width_,hintY+18);text(L"← → "+columnLabel+L"   ／   ↑ ↓ 专辑   ／   ENTER 打开专辑",hintRect,8,400,muted_,false,false,.5f);layoutNode("music-navigation-hint",hintRect);}
}
