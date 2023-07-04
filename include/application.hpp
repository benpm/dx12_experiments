#pragma once

#include <command_queue.hpp>

struct VertexPosColor
{
    XMFLOAT3 position;
    XMFLOAT3 color;
};

class Application
{
public:
    // Number of swap chain back buffer
    constexpr static uint8_t nFrames = 3u;
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
    
    // DirectX 12 Objects
    ComPtr<ID3D12Device2> device;
    CommandQueue cmdQueue;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> backBuffers[nFrames];
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
    XMFLOAT4X4 matModel;
    XMFLOAT4X4 matView;
    XMFLOAT4X4 matProj;
    bool contentLoaded = false;

    // Synchronization objects
    uint64_t frameFenceValues[nFrames] = {};

    bool vsync = true;
    bool tearingSupported = false;
    bool fullscreen = false;

    Application();
    ~Application();

    // Transition a resource
    void transitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
        ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState);
    // Clear a render target view
    void clearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT clearColor[4]);
    // Clear the depth of a depth-stencil view
    void clearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);
    // Create buffer large enough for data, and upload buffer used to transfer to GPU
    void updateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData = nullptr,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    // Resize the depth buffer to match the size of the client area
    void resizeDepthBuffer(uint32_t width, uint32_t height);
    // Create a swap chain
    ComPtr<IDXGISwapChain4> createSwapChain(HWND hWnd, 
        ComPtr<ID3D12CommandQueue> commandQueue, 
        uint32_t width, uint32_t height, uint32_t bufferCount);
    // Create descriptor heap which describes the resources used in rendering
    ComPtr<ID3D12DescriptorHeap> createDescHeap(ComPtr<ID3D12Device2> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    // Render target views describe a resource attached to bind slot during output merge
    void updateRenderTargetViews(ComPtr<ID3D12Device2> device,
        ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    void update();
    // Clear the render target and present the backbuffer
    void render();
    void resize(uint32_t width, uint32_t height);
    void setFullscreen(bool fullscreen);
    void flush();
    void onKeyPressed(UINT key, bool alt);
    bool loadContent();
};