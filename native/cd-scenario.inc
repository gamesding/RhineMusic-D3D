        if(name=="cd-controls"){
            auto control=[this](const char* label,Action input){steps_.push_back({label,.04,[input](App& app,const json&){app.controller(input);return true;}});};
            auto noOverlap=[](const json& r){
                const auto& nodes=r.at("ui").at("layoutNodes");if(!nodes.contains("playback-glass")||!nodes.contains("music-navigation-occupied"))return false;
                auto bottom=[](const json& box){return box[1].get<float>()+box[3].get<float>();};const auto& glass=nodes.at("playback-glass");
                if(bottom(nodes.at("music-navigation-occupied"))>glass[1].get<float>()-10)return false;
                if(nodes.contains("music-navigation-hint")&&bottom(nodes.at("music-navigation-hint"))>glass[1].get<float>()-5)return false;
                const auto& prompts=r.at("ui").at("controllerPrompts");if(glass[2].get<float>()<600&&prompts.value("contextual",false)&&prompts.at("bounds")[3].get<float>()>nodes.at("browse-caption")[1].get<float>()-5)return false;
                if(nodes.contains("cd-eject")){const auto& eject=nodes.at("cd-eject");bool horizontal=eject[0].get<float>()<glass[0].get<float>()+glass[2].get<float>()&&eject[0].get<float>()+eject[2].get<float>()>glass[0].get<float>();if((horizontal&&bottom(eject)>glass[1].get<float>()-5)||bottom(nodes.at("music-navigation-occupied"))>eject[1].get<float>()-8)return false;}
                return true;
            };
            auto picture=[this](const std::string& name){steps_.push_back({name,.7,[name](App& app,const json&){auto directory=executableDirectory()/L"build/cd-controls";std::filesystem::create_directories(directory);app.capture(directory/(wide(name)+L".png"));return true;}});};
            action("skip CD controls intro",0,"skip");
            check("isolated CD UI fixture ready",.2,[](auto& r){return r.at("selectedAlbum").contains("cd")&&r.at("selectedAlbum").value("id",std::string())=="demo-sun";});
            check("selected CD exposes native eject button",.1,[](auto& r){return r.at("ui").at("layoutNodes").contains("cd-eject");});
            control("face-left requests eject rather than starting CD playback",Action::faceLeft);
            check("synthetic CD rejected before hardware operation or vibration",.3,[](auto& r){const auto& eject=r.at("cdEject");return !eject.value("busy",true)&&eject.at("result").contains("error")&&!eject.at("result").value("ejected",true)&&r.at("player").value("currentTrack",json()).is_null()&&r.at("controllers").at("vibration").value("pulses",1u)==0;});
            action("play synthetic track for footer collision checks",.04,"toggle");
            check("test track plays without touching the CD hardware",.5,[](auto& r){return r.at("player").value("playing",false)&&r.at("player").at("currentTrack").value("albumId",std::string())=="demo-sun";});
            control("touchpad center keeps play pause in CD context",Action::play);
            check("CD context center remains pause, not eject",.2,[](auto& r){return r.at("player").value("transport",std::string())=="paused";});
            for(const auto dimensions:std::vector<std::pair<unsigned,unsigned>>{{1280,720},{1920,1080},{2100,900},{900,1600},{800,600},{480,800}}){
                auto size=std::to_string(dimensions.first)+"x"+std::to_string(dimensions.second);resize(("footer resize "+size).c_str(),.03,dimensions.first,dimensions.second);
                for(const std::string theme:{"day","night"}){
                    action(("footer theme "+size+" "+theme).c_str(),.03,"set",{{"key","theme"},{"value",theme}});
                    check(("CD button and navigation clear playback glass "+size+" "+theme).c_str(),.75,noOverlap);picture("footer-cd-"+size+"-"+theme);
                }
            }
            action("open CD detail above transport",.04,"open");
            check("CD detail keeps eject and transport separate",1.5,[](auto& r){const auto& nodes=r.at("ui").at("layoutNodes");return r.value("detail",false)&&nodes.contains("cd-eject")&&nodes.at("cd-eject")[1].get<float>()+nodes.at("cd-eject")[3].get<float>()<nodes.at("playback-glass")[1].get<float>();});
            picture("cd-detail-480x800-night");
            action("hide eject under settings",.03,"panel",{{"panel","settings"}});
            check("CD shortcut hidden while settings owns interaction",.5,[](auto& r){return !r.at("ui").at("layoutNodes").contains("cd-eject");});
            action("close settings",.03,"close-panel");action("return from CD detail",.4,"back");
            steps_.push_back({"select regular album",1.5,[](App& app,const json& r){for(const auto& id:r.at("albumIds"))if(id!="demo-sun"){app.command("locate",{{"albumId",id}});return true;}return false;}});
            check("ordinary album has no eject button",1.8,[](auto& r){return !r.at("selectedAlbum").contains("cd")&&!r.at("ui").at("layoutNodes").contains("cd-eject");});
            control("face-left still resumes music on regular albums",Action::faceLeft);
            check("face-left normal playback mapping survives CD extension",.3,[](auto& r){return r.at("player").value("playing",false);});
            action("return to regular shelf for overlap regression",.03,"back");
            resize("user screenshot aspect ratio",1.5,1920,974);
            for(const std::string theme:{"day","night"}){action(("regular playing shelf theme "+theme).c_str(),.03,"set",{{"key","theme"},{"value",theme}});check(("ordinary navigation clears playback glass "+theme).c_str(),.75,noOverlap);picture("footer-regular-1920x974-"+theme);}
            action("stop isolated footer track",.03,"stop");return;
        }
