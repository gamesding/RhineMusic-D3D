#include "shader-cache.hpp"
#pragma once
#include "platform.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "../vendor/smaa/Textures/AreaTex.h"
#include "../vendor/smaa/Textures/SearchTex.h"
namespace rhine {
class Smaa {
    struct Target {Com<ID3D11Texture2D> image;Com<ID3D11ShaderResourceView> read;Com<ID3D11RenderTargetView> write;};
    Com<ID3D11Device> device_;Com<ID3D11Buffer> metrics_;
    Com<ID3D11ShaderResourceView> area_,search_;Com<ID3D11SamplerState> linear_,point_;
    std::array<Com<ID3D11VertexShader>,3> vertices_;
    std::array<Com<ID3D11PixelShader>,3> pixels_;
    Target edges_,weights_,output_;unsigned width_=0,height_=0;
    Target target(unsigned width,unsigned height,DXGI_FORMAT format){Target result;D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;desc.SampleDesc.Count=1;desc.Format=format;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;check(device_->CreateTexture2D(&desc,nullptr,&result.image),"SMAA target");check(device_->CreateShaderResourceView(result.image.Get(),nullptr,&result.read),"SMAA read view");check(device_->CreateRenderTargetView(result.image.Get(),nullptr,&result.write),"SMAA write view");return result;}
    Com<ID3D11ShaderResourceView> lookup(unsigned width,unsigned height,unsigned pitch,DXGI_FORMAT format,const void* data){Com<ID3D11Texture2D> image;Com<ID3D11ShaderResourceView> result;D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;desc.SampleDesc.Count=1;desc.Format=format;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA content{data,pitch,0};check(device_->CreateTexture2D(&desc,&content,&image),"Official SMAA lookup");check(device_->CreateShaderResourceView(image.Get(),nullptr,&result),"SMAA lookup view");return result;}
public:
    Smaa(ID3D11Device* device,const std::filesystem::path& root):device_(device){
        const char* vertexNames[]={"EdgeVS","WeightVS","BlendVS"};const char* pixelNames[]={"EdgePS","WeightPS","BlendPS"};
        auto compile=[&](const char* entry,const char* profile){if(auto cached=precompiledShader(root,L"smaa.hlsl",entry,profile))return cached;Com<ID3DBlob> code,error;auto result=D3DCompileFromFile((root/L"shaders/smaa.hlsl").c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);if(FAILED(result))throw std::runtime_error(error?std::string(static_cast<const char*>(error->GetBufferPointer()),error->GetBufferSize()):"SMAA shader compilation failed");return code;};
        for(unsigned i=0;i<3;++i){auto vertex=compile(vertexNames[i],"vs_5_0"),pixel=compile(pixelNames[i],"ps_5_0");check(device->CreateVertexShader(vertex->GetBufferPointer(),vertex->GetBufferSize(),nullptr,&vertices_[i]),"SMAA vertex shader");check(device->CreatePixelShader(pixel->GetBufferPointer(),pixel->GetBufferSize(),nullptr,&pixels_[i]),"SMAA pixel shader");}
        D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=16;buffer.Usage=D3D11_USAGE_DEFAULT;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device->CreateBuffer(&buffer,nullptr,&metrics_),"SMAA metrics");
        area_=lookup(AREATEX_WIDTH,AREATEX_HEIGHT,AREATEX_PITCH,DXGI_FORMAT_R8G8_UNORM,areaTexBytes);search_=lookup(SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT,SEARCHTEX_PITCH,DXGI_FORMAT_R8_UNORM,searchTexBytes);
        D3D11_SAMPLER_DESC sample{};sample.AddressU=sample.AddressV=sample.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sample.MaxLOD=D3D11_FLOAT32_MAX;sample.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;check(device->CreateSamplerState(&sample,&linear_),"SMAA linear sampler");sample.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;check(device->CreateSamplerState(&sample,&point_),"SMAA point sampler");
    }
    void run(ID3D11DeviceContext* context,ID3D11Texture2D* color,ID3D11ShaderResourceView* source,unsigned width,unsigned height){
        if(width!=width_||height!=height_){width_=width;height_=height;edges_=target(width,height,DXGI_FORMAT_R8G8_UNORM);weights_=target(width,height,DXGI_FORMAT_R8G8B8A8_UNORM);output_=target(width,height,DXGI_FORMAT_R16G16B16A16_FLOAT);float metrics[]={1.f/width,1.f/height,float(width),float(height)};context->UpdateSubresource(metrics_.Get(),0,nullptr,metrics,0,0);}
        ID3D11Buffer* constant=metrics_.Get();context->VSSetConstantBuffers(3,1,&constant);context->PSSetConstantBuffers(3,1,&constant);context->IASetInputLayout(nullptr);ID3D11SamplerState* samplers[]={linear_.Get(),point_.Get()};context->PSSetSamplers(0,2,samplers);
        const float clear[4]={};context->ClearRenderTargetView(edges_.write.Get(),clear);context->ClearRenderTargetView(weights_.write.Get(),clear);
        Target* passes[]={&edges_,&weights_,&output_};ID3D11ShaderResourceView* reads[]={source,edges_.read.Get(),area_.Get(),search_.Get(),weights_.read.Get()};ID3D11ShaderResourceView* empty[8]={};
        for(unsigned i=0;i<3;++i){context->PSSetShaderResources(0,8,empty);ID3D11RenderTargetView* target=passes[i]->write.Get();context->OMSetRenderTargets(1,&target,nullptr);ID3D11ShaderResourceView* bindings[5]={reads[0],i==1?reads[1]:nullptr,reads[2],reads[3],i==2?reads[4]:nullptr};context->PSSetShaderResources(0,5,bindings);context->VSSetShader(vertices_[i].Get(),nullptr,0);context->PSSetShader(pixels_[i].Get(),nullptr,0);context->Draw(3,0);context->OMSetRenderTargets(0,nullptr,nullptr);}
        context->PSSetShaderResources(0,8,empty);context->CopyResource(color,output_.image.Get());
    }
};
}
