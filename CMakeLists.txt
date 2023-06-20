cmake_minimum_required(VERSION 3.20)
project(dx12_experiments)

find_package(directxtk12 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
add_library(D3D12Hello STATIC D3D12HelloTriangle.cpp DXSample.cpp stdafx.cpp Win32Application.cpp shaders.hlsl)
add_executable(main main.cpp)
target_link_options(main PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:WinMain /NODEFAULTLIB)
include_directories(${CMAKE_SOURCE_DIR})

set_source_files_properties(shaders.hlsl PROPERTIES VS_SHADER_MODEL 6.0 VS_SHADER_FLAGS "-T vs_6_5" VS_SHADER_ENTRYPOINT "VSMain")

target_link_libraries(D3D12Hello  
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
    PUBLIC d3dcompiler.lib
    PUBLIC libcmtd.lib
    PUBLIC libucrtd.lib
    PUBLIC libvcruntimed.lib
    PUBLIC libcpmtd.lib
    PUBLIC atls.lib
)
target_link_libraries(main PRIVATE D3D12Hello)
target_compile_options(main PRIVATE /MTd)
target_compile_options(D3D12Hello PRIVATE /MTd)