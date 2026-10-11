void Ui::controllerGlyph(const std::string& name,D2D1_RECT_F rect,D2D1_COLOR_F paint){
    std::string source,key="kenney-"+name;
    if(!vectors_.contains(key)){std::ifstream file(executableDirectory()/L"runtime/public/controller-prompts"/(wide(name)+L".svg"),std::ios::binary);if(!file)throw std::runtime_error("Controller SVG prompt missing: "+name);source.assign(std::istreambuf_iterator<char>(file),{});}
    vector(key,rect,source,paint);
    controllerPromptEvidence_["glyphs"].push_back({{"name",name},{"rect",json::array({rect.left,rect.top,rect.right,rect.bottom})},{"color",json::array({paint.r,paint.g,paint.b,paint.a})}});
}
void Ui::controllerHints(){
    if(!model_->controllerActive||model_->controllerModel.empty()||model_->booting||model_->viewer||model_->archive||!model_->panel.empty())return;
    bool xbox=model_->controllerModel=="xbox",large=width_>=1700,shortPortrait=portrait_&&!model_->detailSurface&&height_<750;
    const std::string family=xbox?"xbox":"playstation";
    float left=shortPortrait?width_*.5f:portrait_?22.f:compact_?27.f:large?62.f:42.f;
    float top=shortPortrait?111.5f:portrait_?154.f:model_->detailSurface?(compact_?131.f:large?184.f:152.f):(compact_?76.f:large?111.f:89.f);
    float right=portrait_?width_-22:std::min(width_-left,left+(model_->detailSurface?width_*.48f-left:560.f));
    float x=left,y=top;
    auto hint=[&](std::vector<std::string> icons,const std::wstring& caption){
        float width=float(icons.size())*22+5+textWidth(caption,9)+10;
        if(x>left&&x+width>right){x=left;y+=24;}
        fill({x-2,y,x+width-5,y+22},blend(color(0xefede7,.91f),color(0x111b2a,.93f),model_->night),5);
        for(const auto& glyph:icons){controllerGlyph(glyph,{x,y,x+22,y+22},ink_);x+=22;}
        text(caption,{x+4,y+5,right,y+22},9,400,ink_,false,false,0,1.3f);x+=5+textWidth(caption,9)+10;
    };
    if(shortPortrait)hint({family+"_dpad_all"},L"选择");else hint({family+"_dpad_all",family+"_stick_l"},model_->detailSurface?L"选歌":L"选择");
    hint({xbox?"xbox_button_a_outline":"playstation_button_cross_outline"},model_->detailSurface?L"播放":L"打开");
    if(model_->detailSurface)hint({xbox?"xbox_button_b_outline":"playstation_button_circle_outline"},L"返回");
    if(!shortPortrait){hint({xbox?"xbox_button_x_outline":"playstation_button_square_outline"},selectedCd()?L"弹出 CD":L"播放／暂停");hint({xbox?"xbox_button_y_outline":"playstation_button_triangle_outline"},L"搜索");}
    const float helpWidth=textWidth(L"操作 ↗",9)+12;if(x+helpWidth>right){x=left;y+=24;}
    D2D1_RECT_F help{x,y,x+helpWidth,y+22};hits_.push_back({"controller-help","panel",{{"panel","controller-help"}},help,false,""});
    fill(help,blend(color(0xefede7,.91f),color(0x111b2a,.93f),model_->night),5);bool selected=focusVisible_&&focus_=="controller-help";text(L"操作 ↗",{x+3,y+5,help.right,y+22},9,400,selected||contains(help,pointerX_,pointerY_)?accent_:ink_,false,false,0,1.3f);
    if(selected){brush_->SetColor(accent_);renderer_->drawing()->DrawRoundedRectangle(D2D1::RoundedRect(help,4,4),brush_.Get(),1.2f);}
    controllerPromptEvidence_["contextual"]=true;controllerPromptEvidence_["bounds"]=json::array({left,top,right,y+22});
}
void Ui::controllerHelp(float left,float right,float& y){
    std::string current=model_->preferences.value("controllerHelpModel",model_->controllerModel.empty()?std::string("ps4"):model_->controllerModel);
    if(current!="ps4"&&current!="ps5"&&current!="xbox")current="ps4";
    float tabWidth=(right-left-16)/3;
    for(int index=0;index<3;++index){const char* key=index==0?"ps4":index==1?"ps5":"xbox";float x=left+index*(tabWidth+8);panelButton(std::string("controller-model-")+key,index==0?L"PS4":index==1?L"PS5":L"Xbox",{x,y,x+tabWidth,y+34},"set",{{"group","view"},{"key","controllerHelpModel"},{"value",key}},current==key);}
    y+=48;controllerPromptEvidence_["helpModel"]=current;
    bool xbox=current=="xbox";std::string family=xbox?"xbox":"playstation",model=current=="ps5"?"playstation5":"playstation4";
    auto row=[&](std::vector<std::string> icons,const std::wstring& caption,const std::wstring& description=L""){
        float x=left;for(const auto& glyph:icons){controllerGlyph(glyph,{x,y,x+30,y+30},ink_);x+=32;}
        float used=text(caption,{x+8,y+7,right,y+70},11,500,ink_,true,false,0,1.5f)+7;
        if(!description.empty())used+=7+text(description,{x+8,y+used+7,right,y+140},9,400,muted_,true,false,0,1.6f);
        y+=std::max(36.f,used)+14;
    };
    row({xbox?"xbox_button_a_outline":"playstation_button_cross_outline"},L"打开专辑／确认播放",L"进入专辑后，上下选择歌曲；再次确认播放所选歌曲。");
    row({xbox?"xbox_button_b_outline":"playstation_button_circle_outline"},L"返回／关闭面板");
    row({xbox?"xbox_button_x_outline":"playstation_button_square_outline"},L"播放／暂停",L"选中 CD 专辑时，此按键改为弹出光盘；触摸板中间仍为播放／暂停。成功弹出后，已连接手柄短暂震动。");
    row({xbox?"xbox_button_y_outline":"playstation_button_triangle_outline"},L"打开搜索");
    row({family+"_dpad_up",family+"_dpad_down",family+"_stick_l_up",family+"_stick_l_down"},L"上下选择",L"专辑架选专辑；专辑内选歌曲，焦点自动滚动。");
    row({family+"_dpad_left",family+"_dpad_right",family+"_stick_l_left",family+"_stick_l_right"},L"左右切换专辑列");
    row({family+"_stick_r_left",family+"_stick_r_right"},L"旋转专辑模型");
    row({family+"_stick_r_up",family+"_stick_r_down"},L"滚动歌单／曲库");
    row({xbox?"xbox_lb_outline":"playstation_trigger_l1_outline"},L"上一曲");
    row({xbox?"xbox_rb_outline":"playstation_trigger_r1_outline"},L"下一曲");
    row({xbox?"xbox_button_menu_outline":model+"_button_options_outline"},L"打开设置");
    row({family+"_stick_r_press"},L"切换界面焦点",L"右摇杆按下；确认键操作当前控件。");
    if(!xbox){
        row({model+"_touchpad_press_left"},L"触摸板左侧：上一曲",L"左三分之一短按松开切歌；按住约半秒连续快退。");
        row({model+"_touchpad_press_center"},L"触摸板中间：播放／暂停",L"中间三分之一按下切换播放状态。");
        row({model+"_touchpad_press_right"},L"触摸板右侧：下一曲",L"右三分之一短按松开切歌；按住约半秒连续快进。");
    }
    y+=text(L"快退／快进约为 10 倍速度，暂停状态保持不变。按住触摸板时不会旋转模型；未按下时仍可拖动旋转／滚动。窗口失焦或手柄断开会取消该次操作。",{left,y,right,y+220},10,400,muted_,true,false,0,1.7f)+18;
}
