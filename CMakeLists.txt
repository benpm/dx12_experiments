cmake_minimum_required(VERSION 3.20)
project(dx12_experiments)

find_package(directxtk12 CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
add_executable(main
    src/main.cpp
    src/application.cpp
)
target_link_options(main PRIVATE /SUBSYSTEM:WINDOWS)
include_directories("${CMAKE_SOURCE_DIR}/include")

# set_source_files_properties(src/shaders.hlsl PROPERTIES VS_SHADER_MODEL 5.0 VS_SHADER_FLAGS "-T vs_5_0" VS_SHADER_ENTRYPOINT "VSMain")

target_link_libraries(main  
    d3d12.lib dxgi.lib dxguid.lib uuid.lib
    d3dcompiler.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib
    runtimeobject.lib
    Microsoft::DirectXTK12
)