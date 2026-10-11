#pragma once
#include "scene.hpp"
#include "images.hpp"
#include <d2d1_3.h>
#include <dwrite_3.h>
namespace rhine {
struct GraphicsOptions {
    std::string api="auto",upscaler="auto",upscaleQuality="quality",reflections="ssr",reflectionQuality="balanced";
    std::string adapter;
    int shadows=2048,aoSamples=32,anisotropy=16;float scale=1,dof=1,transmission=1,aoResolution=1,pixelRatio=1.5f;bool smaa=false,vsync=true,archive=false,software=false;
};
struct ScreenGlass {XMFLOAT4 rectangle{};XMFLOAT2 pointer{-1000,-1000};float reveal=0,radius=40;};
class Renderer {
    struct State;std::unique_ptr<State> state_;std::filesystem::path capturePath_;
    void writeCapture(const std::filesystem::path& path);
public:
    Renderer(HWND window,const std::filesystem::path& root,const GraphicsOptions& options);
    ~Renderer();
    Renderer(const Renderer&)=delete;Renderer& operator=(const Renderer&)=delete;
    void resize(unsigned width,unsigned height);
    void options(const GraphicsOptions& options);
    void invalidateArtwork();
    XMFLOAT2 jitter()const;
    unsigned renderWidth()const;unsigned renderHeight()const;
    ID2D1DeviceContext5* drawing()const;IDWriteFactory5* writing()const;IDWriteFontCollection* fonts()const;
    void render(const SceneFrame& frame,const json& albums,Images& images,const ScreenGlass& glass,double now,double dt,const std::function<void()>& ui);
    json status()const;
    std::vector<std::pair<int,float>> inspectionParts()const;
    void capture(const std::filesystem::path& path);
};
}
