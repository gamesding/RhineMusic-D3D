#pragma once
#include "platform.hpp"
#include <d3d12.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <nvsdk_ngx_helpers_d3d.h>
#include <ffx_api_loader.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_upscale.h>
namespace rhine {
class Upscaler {
    ID3D12Device* device_=nullptr;
    ID3D11Device* device11_=nullptr;ID3D11DeviceContext* context11_=nullptr;bool spatial11_=false,failed_=false;
    NVSDK_NGX_Parameter* ngx_=nullptr;
    NVSDK_NGX_Handle* dlss_=nullptr;
    ffxFunctions fsr_{};ffxContext context_=nullptr;HMODULE module_=nullptr;
    ffxCreateBackendDX12Desc backendDesc_{};ffxCreateContextDescUpscaleVersion versionDesc_{};ffxCreateContextDescUpscale createDesc_{};
    bool ngxInitialized_=false;
    std::string kind_="native",error_;
    unsigned renderWidth_=0,renderHeight_=0,width_=0,height_=0;
public:
    ~Upscaler();
    void initialize(ID3D12Device* device,unsigned vendor,const std::filesystem::path& directory,const std::string& choice);
    void initialize11(ID3D11Device* device,ID3D11DeviceContext* context,unsigned vendor,const std::filesystem::path& directory,const std::string& choice);
    void resize(ID3D12GraphicsCommandList* command,unsigned width,unsigned height,const std::string& quality,float inputScale=1);
    void releaseFeature();
    void fallbackNative(){releaseFeature();kind_="native";spatial11_=false;failed_=true;renderWidth_=width_;renderHeight_=height_;}
    bool dispatch(ID3D12GraphicsCommandList* command,ID3D12Resource* color,ID3D12Resource* depth,ID3D12Resource* motion,ID3D12Resource* reactive,ID3D12Resource* output,DirectX::XMFLOAT2 jitter,double dt,float fov,bool reset,float nearPlane=5,float farPlane=300);
    bool dispatch11(ID3D11Resource* color,ID3D11Resource* depth,ID3D11Resource* motion,ID3D11Resource* reactive,ID3D11Resource* output,DirectX::XMFLOAT2 jitter,double dt,bool reset);
    bool active()const{return !failed_&&(dlss_||context_||spatial11_);}
    bool spatial11()const{return spatial11_;}
    unsigned renderWidth()const{return renderWidth_;}unsigned renderHeight()const{return renderHeight_;}
    json status()const{return {{"provider",kind_},{"active",active()},{"renderWidth",renderWidth_},{"renderHeight",renderHeight_},{"displayWidth",width_},{"displayHeight",height_},{"error",error_}};}
};
}
