cmake_minimum_required(VERSION 3.24)
project(RhineMusicD3D VERSION 0.1.0 LANGUAGES CXX RC)
if(NOT WIN32 OR NOT MSVC)
  message(FATAL_ERROR "Windows x64 and the installed MSVC toolchain are required")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Configure with -A x64; 32-bit frontend builds are unsupported")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime/drivers/rhine-pad-bridge.dll")
  message(FATAL_ERROR "Missing binary controller bridge; restore the complete source distribution")
endif()
add_custom_target(RhineOriginalUiAssets
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-opencc.py"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-search.py"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-ui.py"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-controls.py"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare-controller-icons.py"
  COMMENT "Prepare original SVG presentation attributes for Direct2D")
add_library(rhine_platform STATIC native/platform.cpp native/controller.cpp native/cd.cpp)
target_include_directories(rhine_platform PUBLIC native vendor/json)
target_compile_definitions(rhine_platform PUBLIC UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN PLATFORM_WINDOWS)
target_compile_options(rhine_platform PUBLIC /utf-8 /W4 /permissive- /EHsc)
target_link_libraries(rhine_platform PUBLIC dxgi d3d12 dxguid d3dcompiler d3d11 d2d1 dwrite windowscodecs
  winhttp shell32 shlwapi user32 gdi32 ole32 uuid xinput hid setupapi winmm advapi32 bcrypt)
add_executable(RhineDeviceProbe native/probe.cpp)
target_link_libraries(RhineDeviceProbe PRIVATE rhine_platform)
add_library(rhine_scene STATIC native/scene.cpp native/mesh.cpp native/service.cpp native/upscaler.cpp native/images.cpp native/renderer.cpp native/ui.cpp native/rays.cpp native/graphics-settings.cpp)
target_include_directories(rhine_scene PUBLIC vendor/dxc/inc vendor/fsr1/ffx-fsr vendor/cgltf vendor/dlss/include vendor/fsr/Kits/FidelityFX/api/include vendor/fsr/Kits/FidelityFX/upscalers/include)
target_link_libraries(rhine_scene PUBLIC rhine_platform comctl32 "${CMAKE_CURRENT_SOURCE_DIR}/vendor/dlss/lib/Windows_x86_64/x64/nvsdk_ngx_s.lib")
add_executable(RhineGraphicsProbe native/graphics-probe.cpp)
target_link_libraries(RhineGraphicsProbe PRIVATE rhine_scene)
add_executable(RhineConfigurationProbe native/configuration-probe.cpp)
target_link_libraries(RhineConfigurationProbe PRIVATE rhine_platform)
set(RHINE_CONFIGURATION_EXE "${CMAKE_CURRENT_BINARY_DIR}/configuration/RhineConfiguration.exe")
set(RHINE_CSC "$ENV{SystemRoot}/Microsoft.NET/Framework64/v4.0.30319/csc.exe")
if(NOT EXISTS "${RHINE_CSC}")
  message(FATAL_ERROR "Windows .NET Framework 4 compiler is required for the standalone configuration program")
endif()
add_custom_command(OUTPUT "${RHINE_CONFIGURATION_EXE}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/configuration"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/build-configuration.py"
    --probe "$<TARGET_FILE:RhineConfigurationProbe>" --output "${RHINE_CONFIGURATION_EXE}"
  DEPENDS RhineConfigurationProbe "$<TARGET_FILE:RhineConfigurationProbe>"
    windows/ConfigurationModel.cs windows/Configuration.cs windows/ConfigurationTests.cs
    windows/configuration.manifest windows/app.ico tools/build-configuration.py
  VERBATIM COMMENT "Build the CPU-rendered standalone graphics configuration program")
add_custom_target(RhineConfiguration ALL DEPENDS "${RHINE_CONFIGURATION_EXE}")
configure_file(windows/native-icon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/native-icon.rc" @ONLY)
add_executable(RhineMusicD3D WIN32 native/main.cpp native/app.cpp native/application.manifest "${CMAKE_CURRENT_BINARY_DIR}/native-icon.rc")
add_dependencies(RhineMusicD3D RhineOriginalUiAssets)
target_link_libraries(RhineMusicD3D PRIVATE rhine_scene psapi)
add_custom_command(TARGET RhineMusicD3D POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:RhineMusicD3D>" "${CMAKE_CURRENT_SOURCE_DIR}/RhineMusicD3D.exe")
add_custom_command(TARGET RhineGraphicsProbe POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:RhineGraphicsProbe>" "${CMAKE_CURRENT_SOURCE_DIR}/RhineGraphicsProbe.exe")
add_executable(RhineAudioHost native/audio-host.cpp)
target_include_directories(RhineAudioHost PRIVATE runtime/vendor/audio/windows-x64/include)
target_compile_options(RhineAudioHost PRIVATE /utf-8 /O2 /fp:precise /EHsc)
target_link_libraries(RhineAudioHost PRIVATE ole32 user32)
add_custom_command(TARGET RhineAudioHost POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:RhineAudioHost>" "${CMAKE_CURRENT_SOURCE_DIR}/runtime/vendor/audio/windows-x64/rhine-audio-host.exe")
add_custom_command(TARGET RhineDeviceProbe POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:RhineDeviceProbe>" "${CMAKE_CURRENT_SOURCE_DIR}/RhineDeviceProbe.exe")
enable_testing()
add_test(NAME graphics_configuration_policy COMMAND "${RHINE_CONFIGURATION_EXE}" --self-test --report "${CMAKE_CURRENT_BINARY_DIR}/configuration-tests/report.json")
add_executable(RhineGraphicsSettingsTests tests/graphics-settings.cpp)
target_link_libraries(RhineGraphicsSettingsTests PRIVATE rhine_scene)
add_test(NAME graphics_startup_preferences COMMAND RhineGraphicsSettingsTests)
add_executable(RhineStateTests tests/native-state.cpp)
target_link_libraries(RhineStateTests PRIVATE rhine_scene)
add_test(NAME native_state_continuity COMMAND RhineStateTests)
add_executable(RhineCdSceneTests tests/cd-scene.cpp)
target_link_libraries(RhineCdSceneTests PRIVATE rhine_scene)
add_test(NAME native_cd_singletons COMMAND RhineCdSceneTests)
add_test(NAME native_devices COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/RhineDeviceProbe.exe" --json "${CMAKE_CURRENT_BINARY_DIR}/device-report.json")
add_executable(RhineControllerTests tests/controller-input.cpp)
target_link_libraries(RhineControllerTests PRIVATE rhine_platform)
add_test(NAME native_controller_transport COMMAND RhineControllerTests)
