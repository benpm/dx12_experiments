cmake_minimum_required(VERSION 3.20)
project(dx12_experiments LANGUAGES CXX C)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# -------------------------------
# Dependencies
# -------------------------------
find_package(directxtk12 CONFIG REQUIRED)
find_package(directxmath CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Configure gainput
set(GAINPUT_BUILD_SHARED OFF)
set(GAINPUT_TESTS OFF)
set(GAINPUT_SAMPLES OFF)
add_subdirectory(lib/gainput)
target_compile_definitions(gainputstatic PRIVATE _CRT_SECURE_NO_WARNINGS)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(gainputstatic PRIVATE -Wno-deprecated-declarations)
endif()

# -------------------------------
# Find DXC shader compiler
# -------------------------------
find_program(DXC_EXECUTABLE dxc
    HINTS "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64"
          "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
)
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "Could not find dxc.exe (DirectX Shader Compiler)")
endif()
message(STATUS "Found DXC: ${DXC_EXECUTABLE}")

# Compile HLSL shaders to .cso
function(compile_shader SHADER_FILE SHADER_TYPE SHADER_MODEL OUTPUT_VAR)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(OUTPUT_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${SHADER_NAME}.cso")
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${DXC_EXECUTABLE} -T ${SHADER_TYPE}_${SHADER_MODEL} -E main
            -Fo ${OUTPUT_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
        COMMENT "Compiling ${SHADER_FILE} -> ${SHADER_NAME}.cso"
        VERBATIM
    )
    set(${OUTPUT_VAR} ${OUTPUT_FILE} PARENT_SCOPE)
endfunction()

# -------------------------------
# Targets
# -------------------------------
add_executable(main
    src/main.cpp
    src/application.cpp
    src/command_queue.cpp
    src/window.cpp
    src/logging.cpp
    src/camera.cpp
    src/input.cpp
    src/upload_buffer.cpp
    src/descriptor_allocator.cpp
)
target_compile_features(main PRIVATE cxx_std_23)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)

# -------------------------------
# Shader compilation
# -------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
compile_shader(src/vertex_shader.hlsl vs 6_0 VS_CSO)
compile_shader(src/pixel_shader.hlsl ps 6_0 PS_CSO)
add_custom_target(shaders DEPENDS ${VS_CSO} ${PS_CSO})
add_dependencies(main shaders)

# -------------------------------
# Compiler options (base + per-config)
# -------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    message(STATUS "Clang detected, configuring compile flags")
    target_compile_options(main PRIVATE
        -Wall -Wno-c++98-compat -fcolor-diagnostics
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "MSVC detected, configuring compile flags")
    target_compile_options(main PRIVATE
        /W3
        $<$<CONFIG:Debug>:/Od /Zi /MDd>
        $<$<CONFIG:Release>:/O2 /MD>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /MD>
    )
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
    target_compile_options(main PRIVATE -Wall)
endif()

# -------------------------------
# Platform & configuration defines
# -------------------------------
target_compile_definitions(main PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

target_compile_definitions(main PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
    $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
    $<$<CONFIG:RelWithDebInfo>:_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
)

# -------------------------------
# Outputs & linking
# -------------------------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(main PRIVATE "LINKER:/SUBSYSTEM:WINDOWS" "LINKER:/ignore:importeddllmain")
else()
    target_link_options(main PRIVATE /SUBSYSTEM:WINDOWS)
endif()

target_link_libraries(main PRIVATE
    d3d12.lib dxgi.lib dxguid.lib uuid.lib
    d3dcompiler.lib
    kernel32.lib user32.lib
    comdlg32.lib advapi32.lib shell32.lib
    ole32.lib oleaut32.lib
    runtimeobject.lib
    Microsoft::DirectXTK12
    spdlog::spdlog
    gainputstatic
)
