#pragma once

#include <gainput/gainput.h>
#include <camera.hpp>
#include <command_queue.hpp>
#include <input.hpp>
#include <unordered_set>

struct VertexPosColor
{
    XMFLOAT3 position;
    XMFLOAT3 color;
};

class Application
{
   public:
    // Number of swap chain back buffer
    constexpr static uint8_t nBuffers = 3u;
    // Use software rasterizer
    bool useWarp = false;
    // Window size
    uint32_t clientWidth = 1280;
    uint32_t clientHeight = 720;
    // DX12 objects initialized
    bool isInitialized = false;
    // Window handle
    HWND hWnd;
    // Window rectangle (used to toggle fullscreen state)
    RECT windowRect;
    // Currently pressed keys
    std::unordered_set<Key> pressedKeys;
    // Currently pressed mouse buttons
    std::unordered_set<MouseButton> pressedMouseButtons;
    // Mouse position
    vec2 mousePos;
    // Mouse delta position
    vec2 mouseDelta;
    // DirectX 12 Objects
    ComPtr<ID3D12Device2> device;
    CommandQueue cmdQueue;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> backBuffers[nBuffers];
    // Render target view descriptor heap
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    UINT rtvDescSize;
    UINT curBackBufIdx;

    ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    ComPtr<ID3D12Resource> depthBuffer;
    // Depth-stencil view descriptor heap needed for depth buffer
    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipelineState;
    D3D12_VIEWPORT viewport;
    // Masks out a region of the screen for rendering
    D3D12_RECT scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    float fov = 45.0f;
    mat4 matModel;
    OrbitCamera cam;
    bool contentLoaded = false;

    // Synchronization objects
    uint64_t frameFenceValues[nBuffers] = {};

    bool vsync = true;
    bool tearingSupported = false;
    bool fullscreen = false;
    bool testMode = false;
    int frameCount = 0;

    // Input
    gainput::InputMap inputMap;
    gainput::DeviceId keyboardID, mouseID, rawMouseID;

    Application();
    ~Application();

    // Transition a resource
    void transitionResource(
        ComPtr<ID3D12GraphicsCommandList2> cmdList,
        ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState
    );
    // Clear a render target view
    void clearRTV(
        ComPtr<ID3D12GraphicsCommandList2> cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        FLOAT clearColor[4]
    );
    // Clear the depth of a depth-stencil view
    void clearDepth(
        ComPtr<ID3D12GraphicsCommandList2> cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        FLOAT depth = 1.0f
    );
    // Create buffer large enough for data, and upload buffer used to transfer
    // to GPU
    void updateBufferResource(
        ComPtr<ID3D12GraphicsCommandList2> cmdList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements,
        size_t elementSize,
        const void* bufferData = nullptr,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE
    );
    // Resize the depth buffer to match the size of the client area
    void resizeDepthBuffer(uint32_t width, uint32_t height);
    // Create a swap chain
    ComPtr<IDXGISwapChain4> createSwapChain();
    // Create descriptor heap which describes the resources used in rendering
    ComPtr<ID3D12DescriptorHeap>
    createDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    // Render target views describe a resource attached to bind slot during
    // output merge
    void updateRenderTargetViews(ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    void update();
    // Clear the render target and present the backbuffer
    void render();
    void setFullscreen(bool val);
    // Flush contents of command queue(s)
    void flush();
    bool loadContent();
    // Called
    void onResize(uint32_t width, uint32_t height);
};
