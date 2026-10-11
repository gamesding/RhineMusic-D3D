void Ui::online(float left,float right,float& y){
    left+=40;controlLeft_=left;controlRight_=right;auto border=blend(color(0x2a312a,.18f),color(0xd0e2f3,.2f),model_->night);
    y+=paragraph(L"在线专辑存放在独立的「在线专辑架」和下面的列表里，不会混入本地音乐库。只有你点击搜索、加入或播放时才会联网；音频和封面由本机服务转发，界面不会直接访问第三方站点。",left,right,y,12,1.85f)+21;
    float x=left;for(int i=0;i<2;++i){auto value=i?L"在线专辑架 · "+std::to_wstring(arrayField(model_->onlineLibrary,"albums").size()):L"本地专辑架";float w=textWidth(value,10)+22;D2D1_RECT_F r{x,y,x+w,y+25};bool active=model_->shelf==(i?"online":"local");fill(r,active?ink_:panelColor_);brush_->SetColor(border);renderer_->drawing()->DrawRectangle(r,brush_.Get(),1);text(value,{x+11,y+5,x+w-10,y+25},10,400,active?blend(color(0xefede7),color(0x162234),model_->night):ink_);Hit hit{i?"online-shelf":"local-shelf","shelf",{{"shelf",i?"online":"local"}},r,false,""};hit.clip=panelClip_;hits_.push_back(std::move(hit));x+=w+7;}panelNode(".genre-filters",{left,y,right,y+25});y+=41;
    auto section=[&](std::wstring value){y+=20;text(std::move(value),{left,y,right,y+16},12,500,ink_);y+=27;};auto end=[&]{y+=20;line(left,y,right,y,border);y+=1;};
    float firstSection=y;section(L"搜索在线曲库");float buttonWidth=textWidth(L"搜索 ↗",16)+36,column=(right-left-buttonWidth-16)/2;
    bool configured=false;for(auto& source:arrayField(model_->onlineSources,"sources"))if(source.value("id",std::string())=="subsonic"&&source.value("configured",false))configured=true;
    json sources=choices({{"internetarchive","Internet Archive"}});if(configured)sources.push_back({{"value","subsonic"},{"label","自有音乐服务"}});
    auto selectedSource=model_->preferences.value("onlineSource",std::string("internetarchive"));if(!configured&&selectedSource=="subsonic")selectedSource="internetarchive";
    selectControl("onlineSource",{left,y,left+column,y+38},sources,selectedSource,"view");
    if(selectedSource!="subsonic")selectControl("onlineCollection",{left+column+8,y,right-buttonWidth-8,y+38},choices({{"","全部音频"},{"opensource_audio","社区上传音频"},{"netlabels","网络厂牌（多为 CC 授权）"},{"etree","现场录音"},{"78rpm","78 转老唱片"}}),model_->preferences.value("onlineCollection",json("")),"view");
    input("online-q",L"",{left,y+46,right-buttonWidth-8,y+92.4f},wide(model_->onlineQuery));panelButton("online-search",L"搜索 ↗",{right-buttonWidth,y,right,y+92.4f},"online-search",{},true,model_->onlineBusy,16);y+=101.4f;
    y+=paragraph(selectedSource=="subsonic"?L"搜索你自己的音乐服务，音频由你的服务提供。":L"搜索 archive.org 的公开音频。每个条目的授权各不相同，未标注授权的内容请自行确认能否使用。",left,right,y,10,1.9f)+9;
    if(!model_->onlineError.empty()){y+=14;y+=paragraph(wide(model_->onlineError),left,right,y)+14;}
    else if(model_->onlineBusy&&arrayField(model_->onlineResults,"items").empty()){y+=14;y+=paragraph(L"正在搜索…",left,right,y)+14;}
    else if(!model_->onlineQuery.empty()&&arrayField(model_->onlineResults,"items").empty()){y+=45;y+=paragraph(L"没有找到匹配的专辑。",left,right,y,12,1.4f)+45;}
    for(auto& item:arrayField(model_->onlineResults,"items")){
        float rowTop=y,addWidth=textWidth(item.value("added",false)?L"已加入":L"加入 ↗",11)+24,copyRight=right-addWidth-13;
        y+=11;y+=text(wide(item.value("title",std::string())),{left,y,copyRight,y+10000},12,500,ink_,true,false,0,1.5f);
        std::wstring subtitle=wide(item.value("artist",std::string()));if(item.contains("year")&&!item["year"].is_null())subtitle+=L" · "+label(item["year"]);
        if(selectedSource=="internetarchive")subtitle+=L" · "+wide(item.value("license",std::string()).empty()?"未标注授权，使用前请查看条目页":"授权见条目页");y+=3;y+=paragraph(subtitle,left,copyRight,y,10,1.5f);
        if(item.contains("url")&&item["url"].is_string()){y+=5;panelButton("online-link-"+item.value("ref",std::string()),L"条目页 ↗",{left,y,copyRight,y+14},"link",{{"url",item["url"]}},false,false,10);y+=14;}
        y+=12;float center=(rowTop+y)*.5f;panelButton("online-add-"+item.value("ref",std::string()),item.value("added",false)?L"已加入":L"加入 ↗",{right-addWidth,center-14,right,center+14},"online-add",{{"ref",item.value("ref",std::string())}},false,item.value("added",false));brush_->SetColor(border);renderer_->drawing()->DrawRectangle(hits_.back().rect,brush_.Get(),1);line(left,y,right,y,border);
    }
    if(model_->onlineResults.value("hasMore",false)){y+=10;panelButton("online-more",model_->onlineBusy?L"正在搜索…":L"更多结果 ↗",{left,y,right,y+22},"online-more",{},false,model_->onlineBusy,16);y+=22;}
    end();panelNode(".panel-section",{left,firstSection,right,y});section(L"我的在线专辑");
    for(auto& album:arrayField(model_->onlineLibrary,"albums")){
        const auto id=album.value("id",std::string());const float buttons=100;cover(album,{left,y+11,left+49,y+60});text(wide(album.value("title",std::string())),{left+62,y+14,right-buttons-13,y+37},12,500,ink_);paragraph(wide(album.value("artist",std::string()))+L" · "+std::to_wstring(arrayField(album,"tracks").size())+L" 首 · "+wide(album.value("folder",std::string())),left+62,right-buttons-13,y+38,10,1.5f);
        panelButton("reveal-online-"+id,L"在架上查看 ↗",{right-buttons,y+7,right,y+35},"online-reveal",{{"id",id}});panelButton("remove-online-"+id,L"移除",{right-buttons,y+41,right,y+69},"online-remove",{{"id",id}});y+=71;line(left,y,right,y,border);
    }
    if(arrayField(model_->onlineLibrary,"albums").empty()){y+=14;y+=paragraph(L"还没有在线专辑。搜索后点“加入”，它们会出现在在线专辑架上。",left,right,y)+14+9;}
    end();section(L"自有音乐服务（Subsonic 兼容）");
    y+=paragraph(L"连接你自己的 Navidrome、Jellyfin（Subsonic 插件）、Airsonic 等服务。地址、账号和密码只保存在本机数据目录的 online.json（密码为明文，建议使用只给播放器用的账号），不会写进项目文件夹。",left,right,y)+9;
    auto subsonic=model_->onlineSources.value("subsonic",json::object());if(!subsonic.is_object())subsonic=json::object();
    for(auto [key,name]:std::array<std::pair<const char*,const wchar_t*>,3>{{{"baseUrl",L"服务地址"},{"username",L"账号"},{"password",L"密码"}}}){
        bool password=key==std::string("password");input(std::string("subsonic-")+key,name,{left,y+23,right,y+69.4f},password?L"":wide(subsonic.value(key,std::string())),false,password);
        if(key==std::string("baseUrl"))text(L"例如 http://192.168.1.10:4533",{right-118,y,right,y+14},9,400,muted_);
        if(password&&subsonic.value("passwordSet",false))text(L"已保存，留空则保持不变",{right-99,y,right,y+14},9,400,muted_);
        y+=69.4f+(password?15.f:13.f);
    }
    panelButton("subsonic-save",L"保存",{left,y,left+58,y+36},"subsonic-save",{},true);panelButton("subsonic-test",L"测试连接",{left+78,y+11,left+122,y+25},"subsonic-test");if(!subsonic.empty())panelButton("subsonic-clear",L"清除",{left+142,y+11,left+164,y+25},"subsonic-clear");y+=36+9+14;
    if(!model_->subsonicStatus.empty())y+=paragraph(wide(model_->subsonicStatus),left,right,y);y+=14;
}
