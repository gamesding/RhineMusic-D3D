#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <json.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <utility>
#include <memory>
#include <array>
#include <algorithm>
#include <functional>
namespace rhine {
using json=nlohmann::json;
template<class T> using Com=Microsoft::WRL::ComPtr<T>;
std::string utf8(const std::wstring& value);
std::wstring wide(const std::string& value);
std::string systemError(DWORD code);
void check(HRESULT result,const char* message);
class HresultError:public std::runtime_error {public:HRESULT code;HresultError(HRESULT result,std::string text):std::runtime_error(std::move(text)),code(result){}};
std::filesystem::path executableDirectory();
class Handle {
    HANDLE value_=INVALID_HANDLE_VALUE;
public:
    Handle()=default;
    explicit Handle(HANDLE value):value_(value){}
    Handle(const Handle&)=delete;
    Handle& operator=(const Handle&)=delete;
    Handle(Handle&& other)noexcept:value_(std::exchange(other.value_,INVALID_HANDLE_VALUE)){}
    Handle& operator=(Handle&& other)noexcept{if(this!=&other){reset();value_=std::exchange(other.value_,INVALID_HANDLE_VALUE);}return *this;}
    ~Handle(){reset();}
    void reset(HANDLE value=INVALID_HANDLE_VALUE){if(value_!=INVALID_HANDLE_VALUE&&value_)CloseHandle(value_);value_=value;}
    HANDLE get()const{return value_;}
    explicit operator bool()const{return value_&&value_!=INVALID_HANDLE_VALUE;}
};
json gpuDevices();
}
