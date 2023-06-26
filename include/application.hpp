#pragma once

#include <command_queue.hpp>

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
    CommandQueue commandQueue;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> backBuffers[nFrames];
    ComPtr<ID3D12GraphicsCommandList2> commandList;
    ComPtr<ID3D12DescriptorHeap> rTVDescriptorHeap;
    UINT rTVDescriptorSize;
    UINT currentBackBufferIndex;

    // Synchronization objects
    uint64_t frameFenceValues[nFrames] = {};

    bool vsync = true;
    bool tearingSupported = false;
    bool fullscreen = false;

    void initialize(HWND hWnd, ComPtr<ID3D12Device2> device, bool tearingSupported);

    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, 
        ComPtr<ID3D12CommandQueue> commandQueue, 
        uint32_t width, uint32_t height, uint32_t bufferCount);
    // Create descriptor heap which describes the resources used in rendering
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    // Render target views describe a resource attached to bind slot during output merge
    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
        ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    void Update();
    // Clear the render target and present the backbuffer
    void Render();
    void Resize(uint32_t width, uint32_t height);
    void SetFullscreen(bool fullscreen);
    void finish();
};