void App::observeCdLibrary(const json& library,const std::string& shelf){
    if(shelf!="local")return;std::set<std::string> next;
    for(const auto& album:library.value("albums",json::array()))if(album.contains("cd")){auto id=album.value("id",std::string());next.insert(id);if(!knownCdIds_.contains(id))pendingCdSelection_=id;}
    knownCdIds_=std::move(next);if(!pendingCdSelection_.empty()&&!knownCdIds_.contains(pendingCdSelection_))pendingCdSelection_.clear();
}
void App::updateCdSelection(){
    if(pendingCdSelection_.empty()||model_.archive||model_.shelf!="local"||!model_.panel.empty()||scene_.booting()||scene_.detail()||model_.detailSurface||presentation_->phase!="archive"||locationPhase_!=LocationPhase::none)return;
    const auto& albums=model_.library.at("albums");for(size_t index=0;index<albums.size();++index)if(albums[index].value("id",std::string())==pendingCdSelection_){scene_.select(int(index),model_.now);model_.selected=scene_.selected();pendingCdSelection_.clear();ui_->endTrackNavigation();notify("已识别 Audio CD，已定位到光盘。");return;}
    pendingCdSelection_.clear();
}
void App::mediaChange(DWORD unitMask,bool inserted){
    bool optical=false;for(int i=0;i<26;++i)if(unitMask&(1u<<i)){std::wstring root(1,wchar_t(L'A'+i));root+=L":\\";if(GetDriveTypeW(root.c_str())==DRIVE_CDROM){optical=true;model_.cdReadDrive=std::string(1,char('A'+i))+":";}}
    if(!optical)return;if(inserted){model_.cdReading=true;model_.cdReadStarted=model_.now;model_.cdReadFinished=-1;}
    rpc("cd:refresh",{},[this](auto){model_.cdReading=false;model_.cdReadFinished=model_.now;},[this](const auto& error){model_.cdReading=false;model_.cdReadFinished=model_.now;notify(error);});
}
