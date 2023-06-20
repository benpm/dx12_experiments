cmake_minimum_required(VERSION 3.20)
project(dx12_experiments)

find_package(directxtk12 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:WinMain")
add_executable(main main.cpp DXSample.cpp stdafx.cpp Win32Application.cpp shaders.hlsl)
include_directories(${CMAKE_SOURCE_DIR})
set_property(TARGET main PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


set_source_files_properties(shaders.hlsl PROPERTIES VS_SHADER_MODEL 6.0)

target_link_libraries(main PRIVATE 
    d3d12.lib dxgi.lib dxguid.lib uuid.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib
    runtimeobject.lib
    Microsoft::DirectXTK12
)