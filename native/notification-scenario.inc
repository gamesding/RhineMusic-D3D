        if(name=="notification-empty"){
            action("skip empty shelf notification intro",0,"skip");
            check("empty shelf matches the ejected last CD",.1,[](const json& r){return r.value("albumCount",1)==0;});
            for(const std::string theme:{"day","night"}){
                action(("empty shelf theme "+theme).c_str(),.03,"set",{{"key","theme"},{"value",theme}});
                steps_.push_back({"show empty shelf eject message "+theme,.03,[](App& app,const json&){app.error("CD 已弹出。");return true;}});
                check(("empty shelf eject message fully visible "+theme).c_str(),.75,[](const json& r){const auto& note=r.at("ui").at("notification");if(note.empty())return false;const auto& b=note.at("bounds");float height=r.at("ui").at("parity").value("logicalHeight",0.f);return note.value("fullyVisible",false)&&note.value("lineCount",0u)==1&&b[1].get<float>()+b[3].get<float>()<height-24;});
                steps_.push_back({"capture empty shelf "+theme,.1,[theme](App& app,const json&){auto directory=executableDirectory()/L"build/notifications";std::filesystem::create_directories(directory);app.capture(directory/(wide("empty-cd-1920x974-"+theme)+L".png"));return true;}});
            }
            return;
        }
        if(name=="notifications"){
            action("skip notification test intro",0,"skip");
            const std::string cd="CD 已弹出。",saved="资料库设置已保存。";
            const std::string longMessage="图形设备初始化失败，已恢复 DX11。请检查设备连接后重试。The selected audio device is unavailable. 音乐库和设置已保留，重新连接后可以继续播放。";
            const std::string multiline="CD 已弹出。\n其他专辑继续播放。\nDSD / Hi-Res / GamesDing";
            const std::string url="读取失败：https://music.example.com/"+std::string(240,'a')+"/album.flac";
            auto show=[this](const std::string& label,const std::string& message){steps_.push_back({label,.03,[message](App& app,const json&){app.error(message);return true;}});};
            auto fits=[](const json& report,bool allowScroll=false){const auto& ui=report.at("ui");const auto& note=ui.at("notification");if(note.empty())return false;const json& rect=note.at("bounds");return (note.value("fullyVisible",false)||allowScroll&&note.value("scrollMax",0.f)>0)&&note.at("textWidth").get<float>()<=note.at("layoutWidth").get<float>()+.1f&&rect[0].get<float>()>=0&&rect[1].get<float>()>=15&&rect[0].get<float>()+rect[2].get<float>()<=ui.at("parity").at("logicalWidth").get<float>()&&rect[1].get<float>()+rect[3].get<float>()<=ui.at("parity").at("logicalHeight").get<float>()-15;};
            auto capture=[this](const std::string& name){steps_.push_back({name,.45,[name](App& app,const json&){auto directory=executableDirectory()/L"build/notifications";std::filesystem::create_directories(directory);app.capture(directory/(wide(name)+L".png"));return true;}});};
            for(const auto dimensions:std::vector<std::pair<unsigned,unsigned>>{{1920,974},{1920,1080},{1280,720},{2100,900},{900,1600},{800,600},{480,800},{360,320}}){
                auto size=std::to_string(dimensions.first)+"x"+std::to_string(dimensions.second);resize(("notification resize "+size).c_str(),.03,dimensions.first,dimensions.second);
                for(const std::string theme:{"day","night"}){
                    action(("notification theme "+size+" "+theme).c_str(),.03,"set",{{"key","theme"},{"value",theme}});
                    for(const auto& message:std::vector<std::pair<std::string,std::string>>{{"cd",cd},{"saved",saved},{"long",longMessage},{"multiline",multiline},{"url",url}}){
                        auto label=message.first+"-"+size+"-"+theme;show("show "+label,message.second);
                        check(("automatic layout contains full text "+label).c_str(),.25,[fits,message](const json& r){if(!fits(r,message.first=="url"))return false;const auto& note=r.at("ui").at("notification");return note.value("text",std::string())==message.second&&(message.first!="cd"||note.value("lineCount",0u)==1)&&(message.first!="multiline"||note.value("lineCount",0u)>=3);});
                        if(message.first=="cd"||message.first=="long")capture(label);
                    }
                }
            }
            std::string overflow;for(int i=0;i<45;++i)overflow+="设备没有响应，请检查数据线连接后重试。";
            show("show a long diagnostic in the minimum viewport",overflow);
            check("long diagnostic stays inside viewport and offers scrolling",.25,[](const json& r){const auto& note=r.at("ui").at("notification");return !note.empty()&&!note.value("fullyVisible",true)&&note.value("scrollMax",0.f)>0&&note.at("bounds")[1].get<float>()>=15;});
            steps_.push_back({"scroll diagnostic without navigating the album",.04,[](App& app,const json& r){const auto& note=r.at("ui").at("notification");const auto& b=note.at("bounds");float scale=r.at("ui").at("parity").value("scale",1.f);app.pointer((b[0].get<float>()+b[2].get<float>()*.5f)*scale,(b[1].get<float>()+b[3].get<float>()*.5f)*scale,false,false,false,0);app.wheel(160);return true;}});
            check("long diagnostic scroll consumed by notification",.1,[](const json& r){return r.at("ui").at("notification").value("scroll",0.f)>0&&r.value("selected",-1)==0;});
            steps_.push_back({"drag diagnostic to its final line",.04,[](App& app,const json& r){const auto& b=r.at("ui").at("notification").at("bounds");float scale=r.at("ui").at("parity").value("scale",1.f),x=(b[0].get<float>()+b[2].get<float>()*.5f)*scale,y=(b[1].get<float>()+b[3].get<float>()*.5f)*scale;app.pointer(x,y,true,false,false,0);app.pointer(x,y-10000,false,false,false,0);app.pointer(x,y-10000,false,true,false,0);app.pointer(0,0,false,false,false,0);return true;}});
            check("touch drag can reach the full diagnostic without click-through",.1,[](const json& r){const auto& note=r.at("ui").at("notification");return std::abs(note.value("scroll",0.f)-note.value("scrollMax",1.f))<.1&&r.value("selected",-1)==0;});
            resize("notification above playing footer",.03,1920,974);action("play isolated notification fixture",.04,"toggle");check("notification footer test really plays a synthetic track",.6,[](const json& r){return r.at("player").value("playing",false)&&r.at("ui").at("layoutNodes").contains("playback-glass");});
            for(const std::string theme:{"day","night"}){
                action(("playing notification theme "+theme).c_str(),.03,"set",{{"key","theme"},{"value",theme}});show("show notification above active footer "+theme,cd);
                check(("notification clears playback glass and navigation "+theme).c_str(),.7,[fits](const json& r){if(!fits(r))return false;const auto& nodes=r.at("ui").at("layoutNodes");if(!nodes.contains("playback-glass"))return false;const auto& note=nodes.at("notification");for(const auto* key:{"playback-glass","music-navigation-occupied","music-navigation-hint"})if(nodes.contains(key)){const auto& b=nodes.at(key);if(note[0].get<float>()<b[0].get<float>()+b[2].get<float>()&&note[0].get<float>()+note[2].get<float>()>b[0].get<float>()&&note[1].get<float>()+note[3].get<float>()>b[1].get<float>()-5)return false;}return true;});capture("playing-cd-1920x974-"+theme);
            }
            action("stop isolated notification fixture",.03,"stop");
            show("fresh notification expires normally",saved);check("notification still uses its initial fade",.05,[](const json& r){return r.at("ui").at("notification").value("opacity",1.f)<1;});
            check("notification fades away after its normal lifetime",5.8,[](const json& r){return r.at("ui").at("notification").empty()&&!r.at("ui").at("layoutNodes").contains("notification");});
            return;
        }
