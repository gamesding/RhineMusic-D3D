#include "upscaler.hpp"
#include <shlobj.h>
namespace rhine {
static std::string ngxError(NVSDK_NGX_Result result){return "NGX error "+std::to_string(static_cast<unsigned>(result));}
Upscaler::~Upscaler(){releaseFeature();if(ngx_){if(device11_)NVSDK_NGX_D3D11_DestroyParameters(ngx_);else NVSDK_NGX_D3D12_DestroyParameters(ngx_);}if(ngxInitialized_){if(device11_)NVSDK_NGX_D3D11_Shutdown1(device11_);else NVSDK_NGX_D3D12_Shutdown1(device_);}if(module_)FreeLibrary(module_);}
void Upscaler::releaseFeature(){if(dlss_){if(device11_)NVSDK_NGX_D3D11_ReleaseFeature(dlss_);else NVSDK_NGX_D3D12_ReleaseFeature(dlss_);dlss_=nullptr;}if(context_){fsr_.DestroyContext(&context_,nullptr);context_=nullptr;}}
void Upscaler::initialize11(ID3D11Device* device,ID3D11DeviceContext* context,unsigned vendor,const std::filesystem::path& directory,const std::string& choice){device11_=device;context11_=context;if(choice=="native")return;
    if(vendor==0x10de&&choice!="fsr"){wchar_t* local=nullptr;check(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local),"NGX data directory");auto logs=std::filesystem::path(local)/L"RhineMusic/D3D/NGX11";CoTaskMemFree(local);std::filesystem::create_directories(logs);auto dllDirectory=directory.wstring();const wchar_t* paths[]={dllDirectory.c_str()};NVSDK_NGX_FeatureCommonInfo info{};info.PathListInfo.Path=paths;info.PathListInfo.Length=1;
        auto result=NVSDK_NGX_D3D11_Init_with_ProjectID("6a1ed11e-0d37-4a73-97e1-63cdffcf4a67",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"0.3.0",logs.c_str(),device,&info);if(NVSDK_NGX_SUCCEED(result)){ngxInitialized_=true;result=NVSDK_NGX_D3D11_GetCapabilityParameters(&ngx_);int supported=0;if(NVSDK_NGX_SUCCEED(result)&&NVSDK_NGX_SUCCEED(ngx_->Get(NVSDK_NGX_Parameter_SuperSampling_Available,&supported))&&supported){kind_="NVIDIA DLSS Super Resolution 310.9.1 (Direct3D 11)";return;}}error_="DLSS unavailable: "+ngxError(result);if(choice=="dlss")return;
    }kind_="AMD FSR 1.0 (Direct3D 11 compatibility)";spatial11_=true;
}
void Upscaler::initialize(ID3D12Device* device,unsigned vendor,const std::filesystem::path& directory,const std::string& choice){
    device_=device;error_.clear();if(choice=="native"){kind_="native";return;}
    if(vendor==0x10de&&choice!="fsr"){
        wchar_t* local=nullptr;check(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local),"NGX data directory");auto logs=std::filesystem::path(local)/L"RhineMusic/D3D/NGX";CoTaskMemFree(local);std::filesystem::create_directories(logs);
        auto dllDirectory=directory.wstring();const wchar_t* paths[]={dllDirectory.c_str()};NVSDK_NGX_FeatureCommonInfo info{};info.PathListInfo.Path=paths;info.PathListInfo.Length=1;
        auto result=NVSDK_NGX_D3D12_Init_with_ProjectID("6a1ed11e-0d37-4a73-97e1-63cdffcf4a67",NVSDK_NGX_ENGINE_TYPE_CUSTOM,"0.3.0",logs.c_str(),device,&info);
        if(NVSDK_NGX_SUCCEED(result)){ngxInitialized_=true;result=NVSDK_NGX_D3D12_GetCapabilityParameters(&ngx_);int supported=0;if(NVSDK_NGX_SUCCEED(result)&&NVSDK_NGX_SUCCEED(ngx_->Get(NVSDK_NGX_Parameter_SuperSampling_Available,&supported))&&supported){kind_="NVIDIA DLSS Super Resolution 310.9.1";return;}}
        error_="DLSS unavailable: "+ngxError(result);if(choice=="dlss"){kind_="native";return;}
    }
    auto dll=directory/L"amd_fidelityfx_upscaler_dx12.dll";module_=LoadLibraryExW(dll.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module_){error_+=(error_.empty()?"":"; ")+std::string("FSR load failed: ")+systemError(GetLastError());kind_="native";return;}
    ffxLoadFunctions(&fsr_,module_);if(!fsr_.CreateContext||!fsr_.DestroyContext||!fsr_.Query||!fsr_.Dispatch){error_="Official FSR API exports are missing";kind_="native";return;}
    kind_="AMD FSR SDK 2.3.0";
}
void Upscaler::resize(ID3D12GraphicsCommandList* command,unsigned width,unsigned height,const std::string& quality,float inputScale){
    releaseFeature();failed_=false;width_=width;height_=height;renderWidth_=width;renderHeight_=height;if(kind_=="native")return;
    float ratio=quality=="performance"?2.f:quality=="balanced"?1.7f:quality=="native-aa"?1.f:1.5f;
    renderWidth_=std::max(1u,unsigned(width/ratio));renderHeight_=std::max(1u,unsigned(height/ratio));
    auto scaledSize=[&](unsigned minW,unsigned minH,unsigned maxW,unsigned maxH){float factor=std::min(inputScale,float(std::sqrt(8294400./double(renderWidth_*uint64_t(renderHeight_)))));factor=std::min({factor,float(maxW)/renderWidth_,float(maxH)/renderHeight_});factor=std::max({factor,float(minW)/renderWidth_,float(minH)/renderHeight_});renderWidth_=std::clamp(unsigned(std::round(renderWidth_*factor)),minW,maxW);renderHeight_=std::clamp(unsigned(std::round(renderHeight_*factor)),minH,maxH);};
    if(ngx_&&kind_.starts_with("NVIDIA")){
        auto mode=quality=="performance"?NVSDK_NGX_PerfQuality_Value_MaxPerf:quality=="balanced"?NVSDK_NGX_PerfQuality_Value_Balanced:quality=="native-aa"?NVSDK_NGX_PerfQuality_Value_DLAA:NVSDK_NGX_PerfQuality_Value_MaxQuality;
        unsigned maxW=renderWidth_,maxH=renderHeight_,minW=renderWidth_,minH=renderHeight_;float sharpness=0;auto settings=NGX_DLSS_GET_OPTIMAL_SETTINGS(ngx_,width,height,mode,&renderWidth_,&renderHeight_,&maxW,&maxH,&minW,&minH,&sharpness);
        if(NVSDK_NGX_FAILED(settings)){error_=ngxError(settings);kind_="native";renderWidth_=width;renderHeight_=height;return;}
        scaledSize(std::max(1u,minW),std::max(1u,minH),std::min(width,std::max(maxW,minW)),std::min(height,std::max(maxH,minH)));
        NVSDK_NGX_DLSS_Create_Params create{};create.Feature.InWidth=renderWidth_;create.Feature.InHeight=renderHeight_;create.Feature.InTargetWidth=width;create.Feature.InTargetHeight=height;create.Feature.InPerfQualityValue=mode;create.InFeatureCreateFlags=NVSDK_NGX_DLSS_Feature_Flags_IsHDR|NVSDK_NGX_DLSS_Feature_Flags_MVLowRes|NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;
        auto result=device11_?NGX_D3D11_CREATE_DLSS_EXT(context11_,&dlss_,ngx_,&create):NGX_D3D12_CREATE_DLSS_EXT(command,1,1,&dlss_,ngx_,&create);if(NVSDK_NGX_FAILED(result)){error_=ngxError(result);kind_="native";renderWidth_=width;renderHeight_=height;}
    }else if(module_){
        scaledSize(1,1,width,height);
        backendDesc_={};backendDesc_.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;backendDesc_.device=device_;
        versionDesc_={};versionDesc_.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;versionDesc_.version=FFX_UPSCALER_VERSION;versionDesc_.header.pNext=&backendDesc_.header;
        createDesc_={};createDesc_.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;createDesc_.header.pNext=&versionDesc_.header;createDesc_.flags=FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE|FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;createDesc_.maxRenderSize={renderWidth_,renderHeight_};createDesc_.maxUpscaleSize={width,height};
        auto result=fsr_.CreateContext(&context_,&createDesc_.header,nullptr);if(result!=FFX_API_RETURN_OK){error_="FSR create failed: "+std::to_string(result);context_=nullptr;kind_="native";renderWidth_=width;renderHeight_=height;}else{ffxQueryGetProviderVersion provider{};provider.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;if(fsr_.Query(&context_,&provider.header)==FFX_API_RETURN_OK&&provider.versionName)kind_=std::string("AMD FSR ")+provider.versionName;}
    }
    else if(spatial11_)scaledSize(1,1,width,height);
}
bool Upscaler::dispatch(ID3D12GraphicsCommandList* command,ID3D12Resource* color,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* reactive,ID3D12Resource* output,DirectX::XMFLOAT2 jitter,double dt,float fov,bool reset,float nearPlane,float farPlane){
    if(dlss_){NVSDK_NGX_D3D12_DLSS_Eval_Params eval{};eval.Feature.pInColor=color;eval.Feature.pInOutput=output;eval.pInDepth=depth;eval.pInMotionVectors=motion;eval.pInTransparencyMask=reactive;eval.pInBiasCurrentColorMask=reactive;eval.InJitterOffsetX=jitter.x;eval.InJitterOffsetY=jitter.y;eval.InMVScaleX=1;eval.InMVScaleY=1;eval.InReset=reset;eval.InRenderSubrectDimensions={renderWidth_,renderHeight_};eval.InFrameTimeDeltaInMsec=float(dt*1000);eval.InPreExposure=1;auto result=NGX_D3D12_EVALUATE_DLSS_EXT(command,dlss_,ngx_,&eval);if(NVSDK_NGX_FAILED(result)){error_=ngxError(result);failed_=true;return false;}return true;}
    if(context_){ffxDispatchDescUpscale eval{};eval.header.type=FFX_API_DISPATCH_DESC_TYPE_UPSCALE;eval.commandList=command;eval.color=ffxApiGetResourceDX12(color);eval.depth=ffxApiGetResourceDX12(depth);eval.motionVectors=ffxApiGetResourceDX12(motion);eval.reactive=ffxApiGetResourceDX12(reactive);eval.transparencyAndComposition=ffxApiGetResourceDX12(reactive);eval.output=ffxApiGetResourceDX12(output,FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);eval.jitterOffset={jitter.x,jitter.y};eval.motionVectorScale={1,1};eval.renderSize={renderWidth_,renderHeight_};eval.upscaleSize={width_,height_};eval.enableSharpening=false;eval.frameTimeDelta=float(dt*1000);eval.preExposure=1;eval.reset=reset;eval.cameraNear=nearPlane;eval.cameraFar=farPlane;eval.cameraFovAngleVertical=fov;eval.viewSpaceToMetersFactor=.01f;auto result=fsr_.Dispatch(&context_,&eval.header);if(result!=FFX_API_RETURN_OK){error_="FSR dispatch failed: "+std::to_string(result);failed_=true;return false;}return true;}
    return false;
}
bool Upscaler::dispatch11(ID3D11Resource* color,ID3D11Resource* depth,ID3D11Resource* motion,ID3D11Resource* reactive,ID3D11Resource* output,DirectX::XMFLOAT2 jitter,double dt,bool reset){if(!dlss_||failed_)return false;NVSDK_NGX_D3D11_DLSS_Eval_Params eval{};eval.Feature.pInColor=color;eval.Feature.pInOutput=output;eval.pInDepth=depth;eval.pInMotionVectors=motion;eval.pInTransparencyMask=reactive;eval.pInBiasCurrentColorMask=reactive;eval.InJitterOffsetX=jitter.x;eval.InJitterOffsetY=jitter.y;eval.InMVScaleX=1;eval.InMVScaleY=1;eval.InReset=reset;eval.InRenderSubrectDimensions={renderWidth_,renderHeight_};eval.InFrameTimeDeltaInMsec=float(dt*1000);eval.InPreExposure=1;auto result=NGX_D3D11_EVALUATE_DLSS_EXT(context11_,dlss_,ngx_,&eval);if(NVSDK_NGX_FAILED(result)){error_=ngxError(result);failed_=true;return false;}return true;}
}
