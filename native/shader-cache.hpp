#pragma once
#include "platform.hpp"
#include <d3dcompiler.h>
#include <bcrypt.h>
namespace rhine {
inline std::string sha256File(const std::filesystem::path& path){
    std::ifstream input(path,std::ios::binary);if(!input)return {};BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::string result;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0){std::array<unsigned char,65536> bytes;while(input){input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());const auto length=input.gcount();if(length&&BCryptHashData(hash,bytes.data(),ULONG(length),0)<0){BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);return {};}}unsigned char digest[32];if(BCryptFinishHash(hash,digest,32,0)>=0){const char* hex="0123456789abcdef";for(auto value:digest){result+=hex[value>>4];result+=hex[value&15];}}BCryptDestroyHash(hash);}BCryptCloseAlgorithmProvider(algorithm,0);return result;
}
inline Com<ID3DBlob> precompiledShader(const std::filesystem::path& root,const wchar_t* source,const char* entry,const char* profile){
    try{std::ifstream file(root/L"shaders/compiled/manifest.json");if(!file)return {};auto manifest=json::parse(file);auto key=utf8(std::filesystem::path(source).stem().wstring())+"-"+entry+"-"+profile;const auto& program=manifest.at("programs").at(key);
        for(auto& [path,digest]:program.at("inputs").items())if(sha256File(root/wide(path))!=digest.get<std::string>())return {};
        auto binary=root/L"shaders/compiled"/wide(key+".cso");if(sha256File(binary)!=program.at("sha256").get<std::string>())return {};Com<ID3DBlob> code;if(SUCCEEDED(D3DReadFileToBlob(binary.c_str(),&code)))return code;
    }catch(...){}return {};
}
}
