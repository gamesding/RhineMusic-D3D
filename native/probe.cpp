#include "controller.hpp"
#include "cd.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include "haptic-probe.inc"
int wmain(int argc,wchar_t** argv){using namespace rhine;try{
    CoInitializeEx(nullptr,COINIT_MULTITHREADED);json report;
    if(argc==4&&std::wstring(argv[1])==L"--haptic-test"&&std::wstring(argv[2])==L"--json"){report=hapticProbe();std::ofstream output{std::filesystem::path(argv[3])};output<<report.dump(2)<<'\n';std::cout<<report.dump(2)<<std::endl;CoUninitialize();return 0;}
    if(argc==4&&std::wstring(argv[1])==L"--eject-cd"){
        std::wstring drive=argv[2];if(drive.size()!=2||drive[1]!=L':'||drive[0]<L'A'||drive[0]>L'Z')throw std::runtime_error("Invalid optical drive");
        CdReader::eject(drive[0],utf8(argv[3]));std::cout<<json({{"ok",true},{"ejected",true},{"drive",utf8(drive)}}).dump()<<std::endl;CoUninitialize();return 0;
    }
    const bool cdOnly=argc==2&&std::wstring(argv[1])==L"--cd-only";
    if(!cdOnly){report["gpus"]=gpuDevices();Controllers controllers;for(int i=0;i<10;++i){controllers.poll(i*.05,false,[](Action){});Sleep(50);}report["controllers"]=controllers.status();}
    auto discs=CdReader::enumerate();report["opticalDrives"]=CdReader::describe(discs);
    if(!cdOnly)for(auto& disc:discs)if(disc.error.empty()){for(auto& track:disc.tracks)if(track.audio){auto sample=CdReader::readAudio(disc.drive,track.startLba,1);report["cdRead"]={{"drive",std::string(1,char(disc.drive))+":"},{"track",track.number},{"bytes",sample.size()},{"sampleStored",false}};break;}if(report.contains("cdRead"))break;}
    report["ok"]=true;if(argc==3&&std::wstring(argv[1])==L"--json"){std::ofstream output{std::filesystem::path(argv[2])};output<<report.dump(2)<<'\n';}
    std::cout<<report.dump(2)<<std::endl;CoUninitialize();return 0;
}catch(const std::exception& error){std::cerr<<json({{"ok",false},{"error",error.what()}}).dump()<<std::endl;return 1;}}
