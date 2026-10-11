static json releaseCd(const json& request){
    const auto drive=request.value("drive",std::string());
    if(drive.size()!=2||drive[0]<'A'||drive[0]>'Z'||drive[1]!=':')throw std::runtime_error("Invalid optical drive");
    std::wstring root{wchar_t(drive[0]),L':',L'\\'};if(GetDriveTypeW(root.c_str())!=DRIVE_CDROM)throw std::runtime_error("Target is not an optical drive");
    HMODULE cd=GetModuleHandleW(L"basscd.dll");if(!cd)throw std::runtime_error("CD decoder is unavailable");
    auto info=reinterpret_cast<decltype(BASS_CD_GetInfo)>(GetProcAddress(cd,"BASS_CD_GetInfo"));
    auto release=reinterpret_cast<decltype(BASS_CD_Release)>(GetProcAddress(cd,"BASS_CD_Release"));
    if(!info||!release)throw std::runtime_error("CD release interface is unavailable");
    for(DWORD index=0;index<26;++index){BASS_CD_INFO device{};if(info(index,&device)&&device.letter==drive[0]-'A'){
        if(!release(index))throw std::runtime_error("无法释放光驱，BASS 错误 "+std::to_string(BASS_ErrorGetCode()));
        return {{"released",true},{"drive",drive}};
    }}
    throw std::runtime_error("CD drive is not available to the decoder");
}
