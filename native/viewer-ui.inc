void Ui::viewer(){
    const auto& albums=arrayField(model_->library,"albums");std::string title="档案模型",id;
    if(model_->selected>=0&&model_->selected<int(albums.size())){title=albums[model_->selected].value("title",title);id=albums[model_->selected].value("id","");}
    const bool portrait=width_/height_<1.05;const float inset=portrait?20:40;
    backControl("viewer-close",model_->archive?L"返回档案":L"返回专辑",inset,32,"viewer-close");
    const float left=portrait?inset:width_*.31f;const float right=width_-inset;
    text(L"RHINE LAB / OBJECT STUDY",{left,portrait?88.f:34.f,right,portrait?111.f:57.f},9,400,muted_);
    text(wide(title),{left,portrait?116.f:65.f,right,portrait?178.f:120.f},portrait?22.f:31.f,600,ink_,true);
    text(wide(id),{left,portrait?179.f:124.f,right,portrait?202.f:146.f},9,400,muted_);
    text(L"360°",{right-85,32,right,87},38,300);
    float surfaceY=portrait?212.f:165.f;bool clear=model_->viewerState.value("clear",true);
    button("viewer-clear",L"清晰",{inset,surfaceY,inset+70,surfaceY+35},"viewer-surface",{{"clear",true}},clear);
    button("viewer-frosted",L"磨砂",{inset+76,surfaceY,inset+146,surfaceY+35},"viewer-surface",{{"clear",false}},!clear);
    if(!portrait){const float x=width_-280;float y=std::min(height_*.35f,height_-140-(model_->archive?6*57.f:3*74.f)-39);y=std::max(225.f,y);text(L"ASSEMBLY / 装配结构",{x,y,width_-40,y+23},9,400,muted_);y+=39;
        const std::array<std::pair<const wchar_t*,const wchar_t*>,3> parts{{{L"封面与玻璃面板",L"COVER PRINT / GLASS"},{L"半透明背板",L"GLASS BACKING"},{L"薄边框",L"THIN GLASS FRAME"}}};
        const std::array<std::pair<const wchar_t*,const wchar_t*>,6> archiveParts{{{L"紧固件",L"FASTENERS"},{L"透明盖板",L"OPTICAL COVER"},{L"折射环组",L"REFRACTIVE RINGS"},{L"光学核心",L"OPTICAL CORE"},{L"信息基板",L"SUBSTRATE"},{L"背板与框架",L"CARRIER"}}};
        for(size_t i=0;i<(model_->archive?archiveParts.size():parts.size());++i){auto part=model_->archive?archiveParts[i]:parts[i];text(number(int(i)+1),{x,y,x+24,y+22},8,400,muted_);text(part.first,{x+34,y,width_-40,y+25},12,500);text(part.second,{x+34,y+28,width_-40,y+48},8,400,muted_);y+=model_->archive?57.f:74.f;}}

    const float y=height_-82;bool exploded=model_->viewerState.value("exploded",false);
    if(!portrait)text(L"拖动旋转   /   ↑ ↓ ← → 平移   /   滚轮缩放",{inset,y+15,width_*.43f,y+41},9,400,muted_);
    const float actions=portrait?inset:width_*.43f;
    button("viewer-explode",model_->archive?L"＋ 拆解档案":L"＋ 拆解专辑",{actions,y,actions+132,y+42},"viewer-explode",{{"explode",true}},exploded);
    button("viewer-assemble",L"− 一键重组",{actions+140,y,actions+272,y+42},"viewer-explode",{{"explode",false}},!exploded);
    button("viewer-reset",L"复位视角 ↗",{portrait?inset:width_-177,portrait?y-52:y,portrait?inset+150:width_-inset,portrait?y-10:y+42},"viewer-reset");
    text(exploded?L"已拆解":L"已组装",{inset,height_-28,right,height_-8},8,400,muted_);
}
