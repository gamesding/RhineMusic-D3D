bool App::selectedCd()const{
    if(model_.archive||viewer_.opened||scene_.booting()||!model_.panel.empty())return false;
    const auto albums=model_.library.value("albums",json::array());int selected=scene_.selected();
    return selected>=0&&selected<int(albums.size())&&albums[selected].contains("cd");
}
void App::ejectCd(){
    if(!selectedCd()||model_.cdEjecting)return;
    auto albums=model_.library.at("albums");auto album=albums.at(scene_.selected());model_.cdEjecting=true;model_.cdEjectResult=json::object();
    rpc("cd:eject",{{"albumId",album.at("id")}},[this](const json& result){
        model_.cdEjecting=false;model_.cdEjectResult=result;
        if(result.value("ejected",false)){model_.cdEjectResult["hapticRequested"]=controllers_.pulse(model_.now);notify("CD 已弹出。");}
    },[this](const std::string& error){model_.cdEjecting=false;model_.cdEjectResult={{"ejected",false},{"error",error}};notify(error);});
}
