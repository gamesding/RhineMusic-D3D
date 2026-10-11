#pragma once
#include "scene.hpp"
#include "mesh.hpp"
#include <d3d12.h>
namespace rhine {
// This component exists only on the explicitly enabled NVIDIA RTX DXR path.
// The shared DX11/SSR renderer remains available on every supported adapter.
class Rays {
    struct State;std::unique_ptr<State> state_;
public:
    Rays(ID3D12Device* device,const std::filesystem::path& root,std::vector<MeshData> assembly,MeshData floor,unsigned mainGeometryCount=0);
    ~Rays();
    void resize(unsigned width,unsigned height,const std::string& quality);
    void run(ID3D12GraphicsCommandList* command,const SceneFrame& scene,unsigned frame,const void* uniforms,size_t uniformBytes,
        ID3D12Resource* depth,ID3D12Resource* normals,ID3D12Resource* opaque,ID3D12Resource* atlas,ID3D12Resource* covers,
        ID3D12Resource* color,ID3D12Resource* motion,ID3D12Resource* environment);
    void copyDiagnostics(ID3D12GraphicsCommandList* command);
    void readDiagnostics();
    json status()const;
};
}
