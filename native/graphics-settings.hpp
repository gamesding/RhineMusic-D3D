#pragma once
#include "renderer.hpp"
namespace rhine {
// Shared by startup and the service preferences path. Never reads music or
// audio settings, and never creates a graphics device while parsing settings.
std::filesystem::path preferenceDirectory();
json loadGraphicsPreferences(const std::filesystem::path& file);
json normalizeGraphicsPreferences(const json& value);
GraphicsOptions graphicsPreferences(GraphicsOptions options,const json& value);
}
