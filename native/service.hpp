#pragma once
#include "platform.hpp"
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
namespace rhine {
class Service {
    Handle job_,process_,input_,output_;
    std::thread reader_;
    std::mutex mutex_;
    std::deque<json> events_;
    std::atomic<bool> stopping_{false};
    unsigned counter_=0;
public:
    Service(const std::filesystem::path& root);
    ~Service();
    unsigned send(std::string operation,json args=json::object());
    std::vector<json> drain();
};
}
