#pragma once
#include "platform.hpp"
#include <d3d11.h>
#include <fstream>
namespace rhine {
inline Com<ID3D11ShaderResourceView> loadEnvironment(ID3D11Device* device,const std::filesystem::path& root){
    std::ifstream input(root/L"runtime/graphics/room-environment.bin",std::ios::binary);if(!input)throw std::runtime_error("Original studio environment missing");unsigned header[4]{};input.read(reinterpret_cast<char*>(header),sizeof(header));if(header[0]!=0x52494342||header[1]!=128||header[2]!=8||header[3]!=6)throw std::runtime_error("Invalid studio environment");
    std::vector<std::vector<unsigned char>> pixels(48);std::vector<D3D11_SUBRESOURCE_DATA> data(48);for(unsigned face=0;face<6;++face)for(unsigned mip=0;mip<8;++mip){unsigned i=face*8+mip,edge=std::max(1u,128u>>mip);pixels[i].resize(edge*edge*8);input.read(reinterpret_cast<char*>(pixels[i].data()),pixels[i].size());if(!input)throw std::runtime_error("Truncated studio environment");data[i]={pixels[i].data(),edge*8,edge*edge*8};}
    D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=128;desc.MipLevels=8;desc.ArraySize=6;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;Com<ID3D11Texture2D> image;check(device->CreateTexture2D(&desc,data.data(),&image),"Upload original studio environment");Com<ID3D11ShaderResourceView> view;D3D11_SHADER_RESOURCE_VIEW_DESC read{};read.Format=desc.Format;read.ViewDimension=D3D11_SRV_DIMENSION_TEXTURECUBE;read.TextureCube.MipLevels=8;check(device->CreateShaderResourceView(image.Get(),&read,&view),"Studio environment cube view");return view;
}
}
