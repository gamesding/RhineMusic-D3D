#include "cd.hpp"
#include <winioctl.h>
#include <ntddcdrm.h>
#include <sstream>
#include <iomanip>
namespace rhine {
static std::wstring devicePath(wchar_t drive){if(drive<L'A'||drive>L'Z')throw std::runtime_error("Invalid optical drive");return std::wstring(L"\\\\.\\")+drive+L":";}
static Handle optical(wchar_t drive){Handle handle(CreateFileW(devicePath(drive).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));if(!handle)throw std::runtime_error("Optical drive: "+systemError(GetLastError()));return handle;}
static int lba(const UCHAR* address){return (int(address[1])*60+int(address[2]))*75+int(address[3])-150;}
CdDisc CdReader::readToc(wchar_t drive){CdDisc result;result.drive=drive;try{auto handle=optical(drive);CDROM_TOC toc{};DWORD bytes=0;if(!DeviceIoControl(handle.get(),IOCTL_CDROM_READ_TOC,nullptr,0,&toc,sizeof(toc),&bytes,nullptr))throw std::runtime_error(systemError(GetLastError()));unsigned count=unsigned(toc.LastTrack)-unsigned(toc.FirstTrack)+1;if(count==0||count>99||bytes<4+(count+1)*sizeof(TRACK_DATA))throw std::runtime_error("Invalid CD table of contents");std::ostringstream identity;identity<<std::hex;for(unsigned index=0;index<count;++index){auto& track=toc.TrackData[index];int first=lba(track.Address),end=lba(toc.TrackData[index+1].Address);if(first<0||end<=first)throw std::runtime_error("Invalid CD track boundary");result.tracks.push_back({track.TrackNumber,first,end-first,(track.Control&4)==0});identity<<first<<'-'<<end<<'-';}result.id=identity.str();}catch(const std::exception& error){result.error=error.what();}return result;}
std::vector<CdDisc> CdReader::enumerate(){std::vector<CdDisc> result;DWORD drives=GetLogicalDrives();for(int i=0;i<26;++i)if(drives&(1u<<i)){wchar_t letter=wchar_t(L'A'+i);std::wstring root=std::wstring(1,letter)+L":\\";if(GetDriveTypeW(root.c_str())==DRIVE_CDROM)result.push_back(readToc(letter));}return result;}
void CdReader::eject(wchar_t drive,const std::string& expectedDiscId){
    devicePath(drive);std::wstring root=std::wstring(1,drive)+L":\\";
    if(GetDriveTypeW(root.c_str())!=DRIVE_CDROM)throw std::runtime_error("所选设备不是光驱");
    auto disc=readToc(drive);if(!disc.error.empty())throw std::runtime_error("无法读取待弹出的 CD："+disc.error);
    if(expectedDiscId.empty()||disc.id!=expectedDiscId)throw std::runtime_error("光盘已更换，请重新选择 CD 专辑");
    auto handle=optical(drive);DWORD bytes=0;
    if(!DeviceIoControl(handle.get(),IOCTL_STORAGE_EJECT_MEDIA,nullptr,0,nullptr,0,&bytes,nullptr))throw std::runtime_error("无法弹出 CD："+systemError(GetLastError()));
}
std::vector<unsigned char> CdReader::readAudio(wchar_t drive,int first,unsigned count){if(first<0||count==0||count>64)throw std::runtime_error("Invalid CD-DA read");auto handle=optical(drive);RAW_READ_INFO info{};info.DiskOffset.QuadPart=LONGLONG(first)*2048;info.SectorCount=count;info.TrackMode=CDDA;std::vector<unsigned char> audio(size_t(count)*2352);DWORD bytes=0;if(!DeviceIoControl(handle.get(),IOCTL_CDROM_RAW_READ,&info,sizeof(info),audio.data(),DWORD(audio.size()),&bytes,nullptr))throw std::runtime_error("Digital CD-DA read: "+systemError(GetLastError()));if(bytes!=audio.size())throw std::runtime_error("Incomplete CD audio sector");return audio;}
json CdReader::describe(const std::vector<CdDisc>& discs){json output=json::array();for(auto& disc:discs){json tracks=json::array();for(auto& track:disc.tracks)tracks.push_back({{"number",track.number},{"startLba",track.startLba},{"sectors",track.sectors},{"duration",track.sectors/75.0},{"audio",track.audio}});output.push_back({{"drive",std::string(1,char(disc.drive))+":"},{"discId",disc.id},{"error",disc.error},{"tracks",tracks}});}return output;}
}
