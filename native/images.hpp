#pragma once
#include "platform.hpp"
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <map>
namespace rhine {
struct Pixels{unsigned width=0,height=0;std::vector<unsigned char> rgba;uint64_t serial=0;std::shared_ptr<Pixels> thumbnail,square;};
class Images {
    struct Job{std::string key,url,title;};
    std::mutex mutex_;std::condition_variable cv_;std::deque<Job> jobs_;
    struct Entry{std::shared_ptr<Pixels> pixels;bool loading=false,failed=false;uint64_t used=0;std::string failure;};
    std::map<std::string,Entry> cache_;std::array<std::thread,2> workers_;
    bool stop_=false;std::string origin_;uint64_t serial_=0,clock_=0;
    Com<IDWriteFactory> write_;Com<IDWriteFontCollection> fonts_;
    void run();
public:
    Images(IDWriteFactory* write,IDWriteFontCollection* fonts);
    ~Images();
    void detachFonts();
    void rebindFonts(IDWriteFactory* write,IDWriteFontCollection* fonts);
    void origin(std::string value){std::lock_guard lock(mutex_);origin_=std::move(value);}
    std::shared_ptr<Pixels> get(const std::string& key,const std::string& url,const std::string& title);
    static std::vector<unsigned char> http(const std::string& url,size_t limit=32*1024*1024);
    json status();
    bool retryFailed();
    static std::shared_ptr<Pixels> tile(const Pixels& source,unsigned size);
};
}
