#include "graphics-settings.hpp"
#include <shlobj.h>
#include <cmath>
namespace rhine {
std::filesystem::path preferenceDirectory(){
    DWORD length=GetEnvironmentVariableW(L"RHINE_APP_HOME",nullptr,0);
    if(length){std::wstring value(length,L'\0');auto count=GetEnvironmentVariableW(L"RHINE_APP_HOME",value.data(),length);if(count&&count<length){value.resize(count);return std::filesystem::absolute(value);}}
    PWSTR roaming=nullptr;check(SHGetKnownFolderPath(FOLDERID_RoamingAppData,0,nullptr,&roaming),"Locate player settings");auto result=std::filesystem::path(roaming)/L"Rhine Music";CoTaskMemFree(roaming);return result;
}
json normalizeGraphicsPreferences(const json& input){
    json result=json::object();if(!input.is_object())return result;
    auto choice=[&](const char* key,std::initializer_list<const char*> values){auto it=input.find(key);if(it==input.end()||!it->is_string())return;auto value=it->get<std::string>();for(auto allowed:values)if(value==allowed){result[key]=value;return;}};
    choice("graphicsApi",{"auto","dx11","dx12"});choice("upscaler",{"auto","native","fsr","dlss"});choice("upscaleQuality",{"quality","balanced","performance","native-aa"});choice("reflections",{"off","ssr","rtx"});choice("reflectionQuality",{"performance","balanced","high"});choice("quality",{"performance","original","high","ultra"});choice("configurationMode",{"automatic","custom"});
    auto software=input.find("graphicsSoftware");if(software!=input.end()&&software->is_boolean())result["graphicsSoftware"]=*software;
    auto adapter=input.find("graphicsAdapter");if(adapter!=input.end()&&adapter->is_string()){auto value=adapter->get<std::string>();if(value.empty()||(value.size()==16&&std::all_of(value.begin(),value.end(),[](unsigned char c){return std::isxdigit(c)!=0;})))result["graphicsAdapter"]=value;}
    json q={{"scale",100},{"pixelRatio",1.5},{"antialias","off"},{"shadows",2048},{"aoSamples",32},{"aoResolution",1},{"depthOfField",100},{"transmission",1},{"anisotropy",16}};
    auto preset=result.value("quality",std::string("original"));if(preset=="performance")q={{"scale",80},{"pixelRatio",1},{"antialias","off"},{"shadows",1024},{"aoSamples",0},{"aoResolution",.5},{"depthOfField",0},{"transmission",.5},{"anisotropy",4}};
    else if(preset=="high"||preset=="ultra"){q["scale"]=preset=="high"?125:150;q["pixelRatio"]=2;q["antialias"]="smaa";q["shadows"]=4096;q["aoSamples"]=preset=="high"?32:64;}
    auto quality=input.find("renderQuality");if(quality!=input.end()&&quality->is_object()){
        auto member=[&](const char* key,std::initializer_list<double> values){auto it=quality->find(key);if(it==quality->end()||!it->is_number())return;double value=it->get<double>();for(auto allowed:values)if(value==allowed){q[key]=value;return;}};
        auto range=[&](const char* key,double low,double high){auto it=quality->find(key);if(it!=quality->end()&&it->is_number()){double value=it->get<double>();if(std::isfinite(value))q[key]=std::clamp(std::round(value/5)*5,low,high);}};
        range("scale",50,200);range("depthOfField",0,150);member("pixelRatio",{1,1.5,2,3});member("shadows",{0,1024,2048,4096});member("aoSamples",{0,16,32,64});member("aoResolution",{.5,.75,1});member("transmission",{.25,.5,.75,1});member("anisotropy",{1,2,4,8,16});auto aa=quality->find("antialias");if(aa!=quality->end()&&aa->is_string()&&(*aa=="off"||*aa=="smaa"))q["antialias"]=*aa;
    }
    if(quality!=input.end()||result.contains("quality"))result["renderQuality"]=q;
    return result;
}
json loadGraphicsPreferences(const std::filesystem::path& file){
    try{if(!std::filesystem::is_regular_file(file)||std::filesystem::file_size(file)>1024*1024)return json::object();std::ifstream source(file,std::ios::binary);if(!source)return json::object();return normalizeGraphicsPreferences(json::parse(source));}catch(const std::exception&){return json::object();}
}
GraphicsOptions graphicsPreferences(GraphicsOptions options,const json& input){
    auto value=normalizeGraphicsPreferences(input);
    options.api=value.value("graphicsApi",options.api);options.upscaler=value.value("upscaler",options.upscaler);options.upscaleQuality=value.value("upscaleQuality",options.upscaleQuality);options.reflections=value.value("reflections",options.reflections);options.reflectionQuality=value.value("reflectionQuality",options.reflectionQuality);options.adapter=value.value("graphicsAdapter",options.adapter);options.software=value.value("graphicsSoftware",options.software);
    if(value.contains("renderQuality")){auto& q=value.at("renderQuality");options.shadows=q.at("shadows").get<int>();options.aoSamples=q.at("aoSamples").get<int>();options.anisotropy=q.at("anisotropy").get<int>();options.scale=q.at("scale").get<float>()/100;options.dof=q.at("depthOfField").get<float>()/100;options.transmission=q.at("transmission").get<float>();options.aoResolution=q.at("aoResolution").get<float>();options.pixelRatio=q.at("pixelRatio").get<float>();options.smaa=q.at("antialias")=="smaa";}
    if(options.software){options.api="dx11";options.upscaler="native";options.reflections="off";}
    return options;
}
}
