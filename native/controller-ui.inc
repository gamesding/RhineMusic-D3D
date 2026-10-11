bool Ui::moveTrackFocus(int direction){
    if(!model_||model_->archive||model_->viewer||!model_->panel.empty()||!model_->detailSurface)return false;
    trackNavigation_=focusVisible_=true;
    if(model_->preferences.value("detailTab",std::string("tracks"))!="tracks"){
        model_->preferences["detailTab"]="tracks";trackFocusRequested_=true;pendingTrackSteps_=0;return true;
    }
    const auto& albums=arrayField(model_->library,"albums");
    if(model_->selected<0||model_->selected>=int(albums.size()))return true;
    const auto identity=albums[model_->selected].value("id",std::string());
    std::vector<Hit> tracks;
    if(model_->ready)for(const auto& hit:hits_)if(hit.key.starts_with("track-")&&!hit.disabled&&hit.args.value("albumId",std::string())==identity)tracks.push_back(hit);
    if(tracks.empty()){
        if(trackFocusRequested_)pendingTrackSteps_=std::clamp(pendingTrackSteps_+direction,-10000,10000);
        trackFocusRequested_=true;return true;
    }
    int current=-1;for(size_t i=0;i<tracks.size();++i)if(tracks[i].key==focus_||(trackFocusRequested_&&tracks[i].key==lastTrackFocus_))current=int(i);
    int base=std::max(0,current);
    if(current<0){auto playing=model_->player.value("currentTrack",json());if(playing.is_object()&&model_->player.value("transport",std::string("idle"))!="idle")for(size_t i=0;i<tracks.size();++i)if(tracks[i].args.value("trackId",std::string())==playing.value("id",std::string()))base=int(i);}
    int step=pendingTrackSteps_+(current>=0||trackFocusRequested_?direction:0);
    focusHit(tracks[std::clamp(base+step,0,int(tracks.size())-1)]);
    pendingTrackSteps_=0;trackFocusRequested_=false;return true;
}
void Ui::resolveTrackFocus(){
    if(!model_||model_->archive||model_->viewer||!model_->panel.empty()||!visiblePanel_.empty()||!model_->detailSurface||!model_->ready||model_->preferences.value("detailTab",std::string("tracks"))!="tracks")return;
    if(trackNavigation_&&trackFocusRequested_){moveTrackFocus(0);return;}
    if(focusVisible_&&focus_.starts_with("track-"))for(const auto& hit:hits_)if(hit.key==focus_){
        if(hit.rect.top<hit.clip.top-.5f||hit.rect.bottom>hit.clip.bottom+.5f){auto copy=hit;focusHit(copy);}break;
    }
}
json Ui::trackFocusEvidence()const{
    json result={{"requested",trackFocusRequested_},{"keyboardVisible",focusVisible_},{"navigation",trackNavigation_},{"pendingSteps",pendingTrackSteps_},{"visible",false}};
    for(const auto& hit:hits_)if(hit.key==focus_&&hit.key.starts_with("track-")){
        result["albumId"]=hit.args.value("albumId",std::string());result["trackId"]=hit.args.value("trackId",std::string());
        result["rect"]=json::array({hit.rect.left,hit.rect.top,hit.rect.right,hit.rect.bottom});
        result["clip"]=json::array({hit.clip.left,hit.clip.top,hit.clip.right,hit.clip.bottom});
        result["visible"]=focusVisible_&&hit.rect.top>=hit.clip.top-.5f&&hit.rect.bottom<=hit.clip.bottom+.5f;
        break;
    }
    return result;
}
