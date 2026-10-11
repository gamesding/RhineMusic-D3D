void Ui::library(float left,float right,float& y){
    left+=40;controlLeft_=left;controlRight_=right;
    const float bodyTop=y;auto border=blend(color(0x2a312a,.18f),color(0xd0e2f3,.2f),model_->night);
    y+=paragraph(L"根目录中的每首单曲各是一张卡片，优先使用自身内嵌封面。子文件夹按专辑展示，优先使用文件夹封面。",left,right,y,12,1.85f)+21;
    std::wstring roots;for(auto& root:arrayField(model_->library,"roots")){if(!roots.empty())roots+=L"\r\n";roots+=wide(root.value("path",""));}
    input("library-roots",L"音乐文件夹",{left,y+23,right,y+110.2f},roots,true);
    text(L"多个目录各占一行",{right-72,y,right,y+14},9,400,muted_);
    panelNode("textarea",{left,y+23,right,y+110.2f});y+=130.2f;
    bool scanning=model_->library.value("scan",json::object()).value("running",false);
    float scanWidth=textWidth(L"保存目录并扫描 ↗",11)+36;
    panelButton("library-scan",L"保存目录并扫描 ↗",{left,y,left+scanWidth,y+36},"library-scan",{},true,scanning);
    panelButton("library-rescan",L"重新扫描",{left+scanWidth+20,y+11,left+scanWidth+64,y+25},"library-rescan",{},false,scanning);y+=36;
    auto scan=model_->library.value("scan",json::object());std::wstring status=scanning?L"正在扫描，已有曲库可以继续浏览…":wide(scan.value("error",std::string()));
    if(status.empty())for(auto& root:arrayField(model_->library,"roots"))if(root.value("status",std::string())=="offline"){if(!status.empty())status+=L"\n";status+=wide(root.value("path",std::string()))+L" 暂时离线，原索引已保留。";}
    if(status.empty())status=scan.value("finishedAt",std::string()).empty()?L"尚未扫描音乐目录。":L"上次扫描 "+wide(scan.value("finishedAt",std::string()));
    y+=14;y+=paragraph(status,left,right,y,10,1.8f)+14;
    float metricsTop=y;line(left,y,right,y,border);y+=18;int count=int(arrayField(model_->library,"albums").size()),tracks=0,genres=0;
    for(auto& album:arrayField(model_->library,"albums"))tracks+=int(arrayField(album,"tracks").size());
    for(auto& genre:arrayField(model_->library,"genres"))if(std::any_of(arrayField(model_->library,"albums").begin(),arrayField(model_->library,"albums").end(),[&](auto& album){return album.value("genreId",std::string())==genre.value("id",std::string());}))++genres;
    for(int i=0;i<3;++i){float x=left+i*(right-left)/3;auto value=std::to_wstring(i==0?count:i==1?tracks:genres);text(value,{x,y,x+(right-left)/3,y+38},29,300,ink_);text(i==0?L"专辑":i==1?L"曲目":L"流派",{x+textWidth(value,29,300)+12,y+20,x+(right-left)/3,y+38},10,400,muted_);}
    y+=55;line(left,y,right,y,border);y+=1;panelNode(".library-metrics",{left,metricsTop,right,y});y+=10;
    auto section=[&](std::wstring name){y+=20;text(std::move(name),{left,y,right,y+16},12,500,ink_);y+=27;};
    auto end=[&]{y+=20;line(left,y,right,y,border);y+=1;};
    float firstSection=y;section(L"在线资料与本地分类");
    y+=paragraph(L"向 MusicBrainz 查询专辑名称与艺术家，补充流派和制作人员；音乐文件留在本机。已有资料使用缓存，人工分类优先保留。",left,right,y)+19;
    const float enrichWidth=textWidth(L"补充缺失的在线资料 ↗",16),rulesWidth=textWidth(L"编辑流派归并规则 ↗",16);
    if(enrichWidth+rulesWidth+40<=right-left){panelButton("enrich-library",L"补充缺失的在线资料 ↗",{left,y,left+enrichWidth,y+22},"enrich",{},false,false,16);panelButton("genre-rules",L"编辑流派归并规则 ↗",{left+enrichWidth+20,y,left+enrichWidth+20+rulesWidth,y+22},"genre-rules",{},false,false,16);y+=22;}
    else {panelButton("enrich-library",L"补充缺失的在线资料 ↗",{left,y,left+enrichWidth,y+22},"enrich",{},false,false,16);y+=32;panelButton("genre-rules",L"编辑流派归并规则 ↗",{left,y,left+rulesWidth,y+22},"genre-rules",{},false,false,16);y+=22;}
    end();panelNode(".panel-section",{left,firstSection,right,y});section(L"封面显示");
    y+=paragraph(L"方形、竖版、横版封面均保持原始比例，完整放入卡片正面。没有封面时显示专辑名称占位，不使用其他专辑的图片。",left,right,y)+9;
    if(!count){y+=10;panelButton("library-demo",L"查看演示封面 ↗",{left,y,right,y+22},"demo",{},false,false,16);y+=22;}
    end();section(L"资料库连接");
    input("metadata-contact",L"MusicBrainz 联系邮箱或项目网址",{left,y+23,right,y+69.4f},wide(model_->config.value("musicBrainzContact",std::string())));y+=78.4f;
    y+=paragraph(L"按 MusicBrainz 要求用于标识本应用的资料请求，不用于注册或订阅。",left,right,y)+15;
    float rowTop=y;toggle("onlineEnabled",L"扫描后自动补充新专辑资料",y,model_->config.value("onlineEnabled",false),false,L"已有缓存不重复查询；断网仍可浏览与播放");panelNode(".settings-row",{left,rowTop,right,y-15});
    y+=10;panelButton("save-metadata",L"保存资料库设置 ↗",{left,y,right,y+22},"save-metadata",{},false,false,16);y+=31;
    y+=paragraph(model_->config.value("musicBrainzConfigured",false)?L"资料库请求标识已配置。":L"尚未配置；本地曲库和播放已可使用。",left,right,y);
    (void)bodyTop;
}

void Ui::search(float left,float right,float& y){
    left+=40;controlLeft_=left;controlRight_=right;input("album-search",L"",{left,y,right,y+46.4f},L"");y+=62.4f;float filtersTop=y;float x=left;auto genre=model_->preferences.value("searchGenre",std::string());json filters=json::array({{{"id",""},{"name","全部"}}});for(auto& item:model_->library.value("genres",json::array())){bool used=false;for(auto& album:model_->library.value("albums",json::array()))if(album.value("genreId",std::string())==item.value("id",std::string()))used=true;if(used)filters.push_back(item);}for(auto& item:filters){auto name=wide(item.value("name",std::string()));float w=textWidth(name,10)+22;if(x+w>right){x=left;y+=32;}D2D1_RECT_F rect{x,y,x+w,y+25};bool active=genre==item.value("id",std::string());if(active)fill(rect,ink_);brush_->SetColor(muted_);renderer_->drawing()->DrawRectangle(rect,brush_.Get(),.5f);text(name,{x+10,y+4,x+w-5,y+24},10,400,active?panelColor_:ink_);Hit hit{"filter-"+item.value("id",std::string()),"set",{{"group","view"},{"key","searchGenre"},{"value",item.value("id",std::string())}},rect,false,""};hit.clip=panelClip_;hits_.push_back(std::move(hit));x+=w+7;}panelNode(".genre-filters",{left,filtersTop,right,y+25});y+=41;
    auto query=searchNormalized(field("album-search"));std::vector<std::wstring> terms;std::wistringstream words(query);for(std::wstring word;words>>word;)terms.push_back(word);
    auto matches=[&](const json& item,const std::string& extra){
        auto display=searchNormalized(wide(item.value("title",std::string())+" "+item.value("artist",std::string())+" "+extra));
        for(const auto& term:terms){if(display.find(term)!=std::wstring::npos)continue;std::wstring latin;for(auto c:term){if(c==L'ü')latin.push_back(L'u');else if(c!=L'\''&&c!=L'-')latin.push_back(c);}bool found=false;for(const auto& key:arrayField(item,"searchPinyin"))if(wide(key.get<std::string>()).find(latin)!=std::wstring::npos){found=true;break;}if(!found)return false;}return true;
    };
    searchEvidence_={{"query",utf8(query)},{"albumIds",json::array()},{"trackIds",json::array()},{"simplifiedTraditional",true},{"pinyin",true}};int found=0;
    for(const auto& album:arrayField(model_->library,"albums")){
        if(!genre.empty()&&album.value("genreId",std::string())!=genre)continue;
        auto row=[&](std::string key,std::string name,std::string subtitle,std::string track){if(!found)panelNode(".album-result",{left,y,right,y+72});cover(album,{left,y+11,left+49,y+60});text(wide(name),{left+62,y+17.5f,right-75,y+38},12,500,ink_,false,false,0,1.5f);text(wide(subtitle),{left+62,y+38.5f,right-75,y+56},10,400,muted_);text(track.empty()?L"专辑":L"歌曲",{right-62,y+25,right-27,y+45},10,400,muted_);text(L"↗",{right-14,y+22,right,y+48},17,400,ink_);Hit hit{std::move(key),"locate",{{"albumId",album.value("id",std::string())},{"trackId",track}},{left,y,right,y+72},false,""};hit.clip=panelClip_;hits_.push_back(std::move(hit));y+=72;line(left,y,right,y,blend(muted_,panelColor_,.7f));if(track.empty())searchEvidence_["albumIds"].push_back(album.value("id",std::string()));else searchEvidence_["trackIds"].push_back(track);++found;};
        std::string genreName;for(const auto& g:arrayField(model_->library,"genres"))if(g.value("id",std::string())==album.value("genreId",std::string()))genreName=g.value("name",std::string());
        if(matches(album,genreName))row("result-"+album.value("id",std::string()),album.value("title",std::string()),album.value("artist",std::string())+" · "+genreName,"");
        if(!terms.empty())for(const auto& track:arrayField(album,"tracks"))if(matches(track,""))row("result-track-"+track.value("id",std::string()),track.value("title",std::string()),track.value("artist",std::string())+" · "+album.value("title",std::string()),track.value("id",std::string()));
    }
    if(!found){text(L"没有找到专辑或歌曲。",{left,y+45,right,y+70},12,400,muted_,false,true);y+=107;}
}
