#pragma once
#include "platform.hpp"
namespace rhine {
struct CdTrack {unsigned number=0;int startLba=0;int sectors=0;bool audio=false;};
struct CdDisc {wchar_t drive=0;std::vector<CdTrack> tracks;std::string id;std::string error;};
class CdReader {
public:
    static std::vector<CdDisc> enumerate();
    static CdDisc readToc(wchar_t drive);
    static std::vector<unsigned char> readAudio(wchar_t drive,int lba,unsigned sectors);
    static void eject(wchar_t drive,const std::string& expectedDiscId);
    static json describe(const std::vector<CdDisc>& discs);
};
}
