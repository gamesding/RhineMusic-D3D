#pragma once
#include "platform.hpp"
#include <DirectXMath.h>
namespace rhine {
struct Vertex{DirectX::XMFLOAT3 position,normal;DirectX::XMFLOAT2 uv;};
struct MeshData{std::string name,surface,part;std::vector<Vertex> vertices;std::vector<unsigned> indices;bool archive=false,arrayEligible=true;DirectX::XMFLOAT4 color{1,1,1,1},finish{.5f,0,0,.04f},attenuation{1,1,1,2},emission{0,0,0,1.46f};};
std::vector<MeshData> loadMusicModel(const std::filesystem::path& file,bool music=true);
}
