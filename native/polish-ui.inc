json Ui::inputEvidence()const{
    json result=json::object();GUITHREADINFO caret{};caret.cbSize=sizeof(caret);bool hasCaret=GetGUIThreadInfo(GetCurrentThreadId(),&caret)!=FALSE;
    for(const auto& [key,item]:inputs_)result[key]={{"focused",item.focused&&GetFocus()==item.handle},{"visible",item.handle&&IsWindowVisible(item.handle)!=FALSE},{"rendered",item.visible&&item.handle&&(GetWindowLongPtrW(item.handle,GWL_STYLE)&WS_VISIBLE)!=0},{"enabled",item.handle&&IsWindowEnabled(item.handle)!=FALSE},{"focusOpacity",item.focusAmount},{"nativeCaret",hasCaret&&caret.hwndCaret==item.handle},{"caretBlinkMs",GetCaretBlinkTime()},{"caretDrawn",item.caretOn&&item.visible},{"caretBounds",{item.caretRect.left,item.caretRect.top,item.caretRect.right,item.caretRect.bottom}},{"selection",{item.selectionStart,item.selectionEnd}},{"compositedEditor",item.bitmap!=nullptr},{"password",item.password},{"maskCharacter",item.handle?unsigned(SendMessageW(item.handle,EM_GETPASSWORDCHAR,0,0)):0u},{"passwordMasked",item.password&&item.handle&&(GetWindowLongPtrW(item.handle,GWL_STYLE)&ES_PASSWORD)!=0&&SendMessageW(item.handle,EM_GETPASSWORDCHAR,0,0)==L'*'},{"textLength",item.handle?GetWindowTextLengthW(item.handle):0},{"editorBounds",{item.position.left/scale_,item.position.top/scale_,(item.position.left+item.position.right)/scale_,(item.position.top+item.position.bottom)/scale_}}};
    return result;
}
void Ui::backControl(std::string key,const std::wstring& caption,float left,float top,std::string action){
    float labelWidth=textWidth(caption,16)+float(caption.size())*.4f,badgeLeft=left+24+labelWidth+13;
    D2D1_RECT_F bounds{left-5,top-1,badgeLeft+40,top+32};bool hovered=contains(bounds,pointerX_,pointerY_),focused=focus_==key&&focusVisible_;
    hits_.push_back({key,std::move(action),json::object(),bounds,false,""});
    if(hovered||pressed_==key)fill(bounds,blend(color(0x232722,.045f),color(0xe9eef2,.065f),model_->night),7);
    if(focused){brush_->SetColor(accent_);renderer_->drawing()->DrawRoundedRectangle(D2D1::RoundedRect(bounds,7,7),brush_.Get(),1.5f);}
    auto paint=hovered||focused?accent_:ink_;
    vector("back-arrow",{left,top+5,left+18,top+23},R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"><path d="m10 6-6 6 6 6M4 12h16"/></svg>)",paint);
    text(caption,{left+24,top,left+24+labelWidth,top+29},16,400,paint,false,false,.4f);
    auto badge=D2D1::RectF(badgeLeft,top+3,badgeLeft+36,top+26);
    auto face=blend(color(0xf2f0e9,.93f),color(0x172435,.94f),model_->night),edge=blend(color(0x6e7567,.27f),color(0xb6c8dc,.32f),model_->night);
    fill({badge.left,badge.top+1.5f,badge.right,badge.bottom+1.5f},blend(color(0x232722,.13f),color(0x000000,.3f),model_->night),5);
    fill(badge,face,5);brush_->SetColor(hovered||focused?accent_:edge);renderer_->drawing()->DrawRoundedRectangle(D2D1::RoundedRect(badge,5,5),brush_.Get(),1);
    if(model_->controllerActive&&!model_->controllerModel.empty())controllerGlyph(model_->controllerModel=="xbox"?"xbox_button_b_outline":"playstation_button_circle_outline",{badge.left+5,badge.top-1,badge.right-5,badge.bottom+2},hovered||focused?accent_:ink_);else text(L"ESC",{badge.left+2,badge.top+5,badge.right-2,badge.bottom},9,600,hovered||focused?accent_:muted_,false,true,.7f,1.2f);
    layoutNode(key+"-keycap",badge);
}
