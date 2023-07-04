cmake_minimum_required(VERSION 3.20)
project(dx12_experiments)

find_package(directxtk12 CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
add_executable(main
    "src/main.cpp"
    "src/application.cpp"
    "src/command_queue.cpp"
    "src/window.cpp"
    "src/logging.cpp"
    
    "src/vertex_shader.hlsl"
    "src/pixel_shader.hlsl"
)
target_link_options(main PRIVATE /SUBSYSTEM:WINDOWS)
target_compile_definitions(main PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
include_directories("${CMAKE_SOURCE_DIR}/include")

# Enable all warnings
target_compile_options(main PRIVATE /W4 /WX)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(main PRIVATE _DEBUG)
endif()

set_source_files_properties("src/vertex_shader.hlsl" PROPERTIES
    VS_SHADER_MODEL 5.1
    VS_SHADER_TYPE Vertex
    VS_SHADER_ENTRYPOINT "main")

set_source_files_properties("src/pixel_shader.hlsl" PROPERTIES
    VS_SHADER_MODEL 5.1
    VS_SHADER_TYPE Pixel
    VS_SHADER_ENTRYPOINT "main")

target_link_libraries(main  
    d3d12.lib dxgi.lib dxguid.lib uuid.lib
    d3dcompiler.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib
    runtimeobject.lib
    Microsoft::DirectXTK12
    spdlog::spdlog
)