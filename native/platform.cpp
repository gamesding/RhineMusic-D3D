#include "platform.hpp"
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3d11.h>
#include <sstream>
namespace rhine {
std::string utf8(const std::wstring& value){if(value.empty())return {};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);if(!n)throw std::runtime_error("Invalid UTF-16");std::string result(n,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),result.data(),n,nullptr,nullptr);return result;}
std::wstring wide(const std::string& value){if(value.empty())return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0);if(!n)throw std::runtime_error("Invalid UTF-8");std::wstring result(n,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),result.data(),n);return result;}
std::string systemError(DWORD code){wchar_t* buffer=nullptr;FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,code,0,reinterpret_cast<LPWSTR>(&buffer),0,nullptr);std::string result=buffer?utf8(buffer):"Windows error "+std::to_string(code);LocalFree(buffer);return result;}
void check(HRESULT result,const char* message){if(FAILED(result)){std::ostringstream text;text<<message<<" (0x"<<std::hex<<static_cast<unsigned long>(result)<<"): "<<systemError(result);throw HresultError(result,text.str());}}
std::filesystem::path executableDirectory(){std::wstring path(32768,L'\0');DWORD n=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));if(n==0||n>=path.size())throw std::runtime_error("Cannot locate executable");path.resize(n);return std::filesystem::path(path).parent_path();}
json gpuDevices(){
    Com<IDXGIFactory1> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");json devices=json::array();
    for(UINT index=0;;++index){Com<IDXGIAdapter1> adapter;auto result=factory->EnumAdapters1(index,&adapter);if(result==DXGI_ERROR_NOT_FOUND)break;check(result,"Enumerate adapter");
        DXGI_ADAPTER_DESC1 info{};check(adapter->GetDesc1(&info),"Adapter description");
        Com<ID3D11Device> device11;D3D_FEATURE_LEVEL level{};auto dx11=D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device11,&level,nullptr);
        Com<ID3D12Device> device;auto dx12=D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device));unsigned tier=0;bool inlineRays=false;
        if(SUCCEEDED(dx12)){D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};if(SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options))))tier=unsigned(options.RaytracingTier);D3D12_FEATURE_DATA_SHADER_MODEL shader{D3D_SHADER_MODEL_6_5};inlineRays=tier>=D3D12_RAYTRACING_TIER_1_1&&SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&shader,sizeof(shader)))&&shader.HighestShaderModel>=D3D_SHADER_MODEL_6_5;}
        devices.push_back({{"name",utf8(info.Description)},{"vendorId",info.VendorId},{"deviceId",info.DeviceId},{"dedicatedBytes",info.DedicatedVideoMemory},{"software",bool(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)},{"d3d11",SUCCEEDED(dx11)},{"featureLevel11",unsigned(level)},{"d3d12",SUCCEEDED(dx12)},{"raytracingTier",tier},{"inlineRaytracing",inlineRays},{"spectralCausticsEligible",inlineRays&&info.VendorId==0x10de}});
    }return devices;
}
}
