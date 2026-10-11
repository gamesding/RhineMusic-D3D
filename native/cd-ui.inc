bool Ui::selectedCd()const{
    if(!model_||model_->archive||model_->viewer||model_->booting||!model_->panel.empty())return false;
    const auto& albums=arrayField(model_->library,"albums");
    return model_->selected>=0&&model_->selected<int(albums.size())&&albums[model_->selected].contains("cd");
}
D2D1_RECT_F Ui::cdEjectRect()const{
    float right=width_-(portrait_?22.f:compact_?27.f:42.f),bottom=height_-44,top=bottom-44;
    if(playbackVisible_){const auto bar=playbackLayout(width_,height_);if(right-142<bar.right&&right>bar.left&&bottom>bar.top&&top<bar.bottom){bottom=bar.top-10;top=bottom-44;}}
    return {right-142,top,right,bottom};
}
void Ui::cdEject(){
    if(!selectedCd())return;const auto rect=cdEjectRect();bool busy=model_->cdEjecting,selected=focusVisible_&&focus_=="cd-eject",hover=contains(rect,pointerX_,pointerY_);
    hits_.push_back({"cd-eject","cd-eject",{},rect,busy,""});layoutNode("cd-eject",rect);
    fill(rect,blend(color(0xefede7,.95f),color(0x111b2a,.95f),model_->night),8);
    auto paint=busy?muted_:selected||hover?accent_:ink_;brush_->SetColor(selected?accent_:blend(muted_,panelColor_,.55f));renderer_->drawing()->DrawRoundedRectangle(D2D1::RoundedRect(rect,8,8),brush_.Get(),selected?1.3f:1.f);
    const std::string key="cd-player-eject";std::string source;if(!vectors_.contains(key)){std::ifstream file(executableDirectory()/L"runtime/public/native-icons/player-eject.svg",std::ios::binary);if(!file)throw std::runtime_error("Official eject SVG missing");source.assign(std::istreambuf_iterator<char>(file),{});}
    vector(key,{rect.left+12,rect.top+10,rect.left+36,rect.top+34},source,paint);
    text(busy?L"弹出中…":L"弹出",{rect.left+44,rect.top+13,rect.right-34,rect.bottom},12,500,paint,false,false,0,1.3f);
    if(model_->controllerActive&&!model_->controllerModel.empty())controllerGlyph(model_->controllerModel=="xbox"?"xbox_button_x_outline":"playstation_button_square_outline",{rect.right-30,rect.top+11,rect.right-8,rect.top+33},paint);
}
