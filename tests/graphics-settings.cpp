#include "graphics-settings.hpp"
#include <iostream>
using namespace rhine;
static void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    auto invalid=normalizeGraphicsPreferences({{"graphicsApi",42},{"graphicsSoftware","true"},{"graphicsAdapter","bad/path"},{"renderQuality",{{"shadows",-1},{"scale",999},{"depthOfField",-2},{"aoSamples","64"},{"transmission",-1}}},{"output",{{"backend","asio"}}}});
    require(!invalid.contains("graphicsApi")&&!invalid.contains("graphicsSoftware")&&!invalid.contains("graphicsAdapter")&&!invalid.contains("output"),"Invalid/private settings leaked into startup graphics");
    require(invalid["renderQuality"]["scale"]==200&&invalid["renderQuality"]["depthOfField"]==0&&invalid["renderQuality"]["shadows"]==2048&&invalid["renderQuality"]["aoSamples"]==32,"Invalid graphics ranges did not normalize");
    auto directory=std::filesystem::temp_directory_path()/(L"RhineGraphicsSettings-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(directory);auto file=directory/L"preferences.json";
    json saved={{"graphicsApi","dx11"},{"upscaler","fsr"},{"graphicsAdapter","0000000000001234"},{"quality","performance"},{"renderQuality",{{"scale",65},{"shadows",0},{"aoSamples",0},{"transmission",.25}}},{"theme","night"},{"output",{{"backend","asio"}}}};
    {std::ofstream output(file);output<<saved.dump();}auto parsed=loadGraphicsPreferences(file);auto options=graphicsPreferences({},parsed);
    require(options.api=="dx11"&&options.upscaler=="fsr"&&options.shadows==0&&options.aoSamples==0&&std::abs(options.scale-.65f)<.001f&&options.transmission==.25f,"Saved preferences were not applied before device creation");
    require(options.adapter=="0000000000001234"&&!parsed.contains("theme")&&!parsed.contains("output"),"Adapter selection or settings boundary failed");
    auto software=graphicsPreferences({},{{"graphicsSoftware",true},{"graphicsApi","dx12"},{"upscaler","dlss"},{"reflections","rtx"}});require(software.software&&software.api=="dx11"&&software.upscaler=="native"&&software.reflections=="off","Software mode failed to disable incompatible features");
    {std::ofstream output(file);output<<"{bad JSON";}require(loadGraphicsPreferences(file).empty(),"Malformed startup preferences prevented recovery");std::filesystem::remove(file);std::filesystem::remove(directory);
    require(loadGraphicsPreferences(file).empty(),"Missing preferences were not safe");std::cout<<"Startup graphics preferences, quality presets, invalid data and software compatibility passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
