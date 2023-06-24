#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <chrono>
#include <string>
#include <cassert>
#include <wrl.h>
#include <shellapi.h>

using Microsoft::WRL::ComPtr;

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

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
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGISwapChain4> swapChain;
    ComPtr<ID3D12Resource> backBuffers[nFrames];
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12CommandAllocator> commandAllocators[nFrames];
    ComPtr<ID3D12DescriptorHeap> rTVDescriptorHeap;
    UINT rTVDescriptorSize;
    UINT currentBackBufferIndex;

    // Synchronization objects
    ComPtr<ID3D12Fence> fence;
    uint64_t fenceValue = 0;
    uint64_t frameFenceValues[nFrames] = {};
    HANDLE fenceEvent;

    bool vsync = true;
    bool tearingSupported = false;
    bool fullscreen = false;

    void initialize(HWND hWnd, ComPtr<ID3D12Device2> device, bool tearingSupported);

    ComPtr<ID3D12CommandQueue> CreateCommandQueue(
        ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type );
    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, 
        ComPtr<ID3D12CommandQueue> commandQueue, 
        uint32_t width, uint32_t height, uint32_t bufferCount);
    // Create descriptor heap which describes the resources used in rendering
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    // Render target views describe a resource attached to bind slot during output merge
    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
        ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    // Command allocator is backing mem for command lists
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
        ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    // Command lists record commands and are executed by command queue
    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
        ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
    // Fences allow sync between CPU/GPU
    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
    HANDLE CreateEventHandle();
    uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue);
    void WaitForFenceValue(
        ComPtr<ID3D12Fence> fence, uint64_t fenceValue,
        HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue, HANDLE fenceEvent);
    void Update();
    // Clear the render target and present the backbuffer
    void Render();
    void Resize(uint32_t width, uint32_t height);
    void SetFullscreen(bool fullscreen);
    void finish();
};