#include "platform.hpp"
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace rhine;
static std::string identity(LUID id){std::ostringstream value;value<<std::hex<<std::setfill('0')<<std::setw(8)<<static_cast<unsigned long>(id.HighPart)<<std::setw(8)<<id.LowPart;return value.str();}
int wmain(int argc,wchar_t** argv){
    try{
        if(argc!=3||std::wstring(argv[1])!=L"--report")throw std::runtime_error("Expected --report <file>");
        check(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"Configuration probe COM");
        Com<IDXGIFactory1> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"Configuration DXGI factory");json devices=json::array();
        for(UINT index=0;index<32;++index){
            Com<IDXGIAdapter1> adapter;auto enumerated=factory->EnumAdapters1(index,&adapter);if(enumerated==DXGI_ERROR_NOT_FOUND)break;if(FAILED(enumerated))continue;
            DXGI_ADAPTER_DESC1 info{};if(FAILED(adapter->GetDesc1(&info))||(info.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))continue;
            Com<ID3D11Device> device11;Com<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level{};auto result11=D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device11,&level,&context);
            bool dx11=SUCCEEDED(result11)&&level>=D3D_FEATURE_LEVEL_11_0;
            Com<ID3D12Device> device12;auto result12=D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device12));bool dx12=false;unsigned tier=0;bool inlineRays=false;
            if(SUCCEEDED(result12)){
                D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};if(SUCCEEDED(device12->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&features,sizeof(features))))tier=unsigned(features.RaytracingTier);
                D3D12_FEATURE_DATA_SHADER_MODEL shader{D3D_SHADER_MODEL_6_5};inlineRays=tier>=D3D12_RAYTRACING_TIER_1_1&&SUCCEEDED(device12->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&shader,sizeof(shader)))&&shader.HighestShaderModel>=D3D_SHADER_MODEL_6_5;
                Com<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC desc{};desc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
                if(SUCCEEDED(device12->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)))){IUnknown* queues[]={queue.Get()};Com<ID3D11Device> bridgeDevice;Com<ID3D11DeviceContext> bridgeContext;auto bridged=D3D11On12CreateDevice(device12.Get(),D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,queues,1,0,&bridgeDevice,&bridgeContext,nullptr);Com<ID3D11On12Device2> bridge;dx12=SUCCEEDED(bridged)&&SUCCEEDED(bridgeDevice.As(&bridge));}
            }
            std::string name=utf8(info.Description);bool rtx=name.find("RTX")!=std::string::npos||name.find("rtx")!=std::string::npos;
            devices.push_back({{"name",name},{"luid",identity(info.AdapterLuid)},{"vendorId",info.VendorId},{"deviceId",info.DeviceId},{"dedicatedBytes",info.DedicatedVideoMemory},{"sharedBytes",info.SharedSystemMemory},{"software",false},{"d3d11",dx11},{"d3d12",dx12},{"d3d12Device",SUCCEEDED(result12)},{"featureLevel11",unsigned(level)},{"raytracingTier",tier},{"inlineRaytracing",inlineRays},{"spectralCausticsEligible",dx12&&inlineRays&&info.VendorId==0x10de&&rtx},{"dlssEligible",dx11&&info.VendorId==0x10de&&rtx}});
        }
        Com<ID3D11Device> warp;D3D_FEATURE_LEVEL warpLevel{};bool software=SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&warp,&warpLevel,nullptr))&&warpLevel>=D3D_FEATURE_LEVEL_11_0;
        if(software)devices.push_back({{"name","Windows 软件渲染 · WARP"},{"luid",""},{"vendorId",0},{"dedicatedBytes",0},{"sharedBytes",0},{"software",true},{"d3d11",true},{"d3d12",false},{"raytracingTier",0},{"inlineRaytracing",false},{"spectralCausticsEligible",false},{"dlssEligible",false}});
        MEMORYSTATUSEX memory{sizeof(memory)};GlobalMemoryStatusEx(&memory);SYSTEM_POWER_STATUS power{};GetSystemPowerStatus(&power);
        json report={{"ok",true},{"adapters",devices},{"memoryBytes",memory.ullTotalPhys},{"availableMemoryBytes",memory.ullAvailPhys},{"logicalProcessors",GetActiveProcessorCount(ALL_PROCESSOR_GROUPS)},{"onBattery",power.ACLineStatus==0},{"softwareAvailable",software},{"version","0.1"}};
        auto file=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(file.parent_path());std::ofstream output(file,std::ios::binary|std::ios::trunc);if(!output)throw std::runtime_error("Cannot write configuration report");output<<report.dump(2);output.close();CoUninitialize();return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
