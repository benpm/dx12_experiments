#include <application.hpp>

void Application::initialize(HWND hWnd, ComPtr<ID3D12Device2> device, bool tearingSupported)
{
    this->hWnd = hWnd;
    this->device = device;
    this->tearingSupported = tearingSupported;

    this->commandQueue = this->CreateCommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    this->swapChain = this->CreateSwapChain(hWnd, this->commandQueue,
        this->clientWidth, this->clientHeight, this->nFrames);

    this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

    this->rTVDescriptorHeap = this->CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, this->nFrames);
    this->rTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    this->UpdateRenderTargetViews(device, this->swapChain, this->rTVDescriptorHeap);
    for (int i = 0; i < this->nFrames; ++i) {
        this->commandAllocators[i] = this->CreateCommandAllocator(this->device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    this->commandList = this->CreateCommandList(this->device,
        this->commandAllocators[this->currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

    this->fence = this->CreateFence(this->device);
    this->fenceEvent = CreateEventHandle();
    
    this->isInitialized = true;
}

// Create the command queue using the given device, of given type (direct, compute, copy)
ComPtr<ID3D12CommandQueue> Application::CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type )
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
 
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type =     type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags =    D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
 
    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
 
    return d3d12CommandQueue;
}

// Create swap chain which describes the sequence of buffers used for rendering
ComPtr<IDXGISwapChain4> Application::CreateSwapChain(HWND hWnd, 
    ComPtr<ID3D12CommandQueue> commandQueue, 
    uint32_t width, uint32_t height, uint32_t bufferCount )
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
    
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = this->tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    ComPtr<IDXGISwapChain1> swapChain1;
    ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(ComPtr<ID3D12Device2> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Application::UpdateRenderTargetViews(ComPtr<ID3D12Device2> device,
    ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < this->nFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        this->backBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

ComPtr<ID3D12CommandAllocator> Application::CreateCommandAllocator(
    ComPtr<ID3D12Device2> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> Application::CreateCommandList(ComPtr<ID3D12Device2> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    
    ThrowIfFailed(commandList->Close());

    return commandList;
}

ComPtr<ID3D12Fence> Application::CreateFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

    return fence;
}

HANDLE Application::CreateEventHandle()
{
    HANDLE fenceEvent;
    
    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

uint64_t Application::Signal(
    ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));

    return fenceValueForSignal;
}

void Application::WaitForFenceValue(
    ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
    std::chrono::milliseconds duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void Application::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue, HANDLE fenceEvent )
{
    uint64_t fenceValueForSignal = this->Signal(commandQueue, fence, fenceValue);
    this->WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void Application::Update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;
    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        char buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        sprintf_s(buffer, 500, "FPS: %f\n", fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}

void Application::Render()
{
    auto commandAllocator = this->commandAllocators[this->currentBackBufferIndex];
    auto backBuffer = this->backBuffers[this->currentBackBufferIndex];

    commandAllocator->Reset();
    this->commandList->Reset(commandAllocator.Get(), nullptr);
    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        this->commandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(this->rTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            this->currentBackBufferIndex, this->rTVDescriptorSize);

        this->commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }
    // Present
    {
        // Transition backbuffer to present state
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        this->commandList->ResourceBarrier(1, &barrier);
        ThrowIfFailed(this->commandList->Close());

        ID3D12CommandList* const commandLists[] = {
            this->commandList.Get()
        };
        this->commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
        UINT syncInterval = this->vsync ? 1 : 0;
        UINT presentFlags = this->tearingSupported && !this->vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        ThrowIfFailed(this->swapChain->Present(syncInterval, presentFlags));

        // Signal frame completion and wait
        this->frameFenceValues[this->currentBackBufferIndex] = Signal(this->commandQueue, this->fence, this->fenceValue);
        this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(this->fence, this->frameFenceValues[this->currentBackBufferIndex], this->fenceEvent);
    }
}

void Application::Resize(uint32_t width, uint32_t height)
{
    if (this->clientWidth != width || this->clientHeight != height)
    {
        // Don't allow 0 size swap chain back buffers.
        this->clientWidth = std::max(1u, width);
        this->clientHeight = std::max(1u, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        //  are not being referenced by an in-flight command list.
        Flush(this->commandQueue, this->fence, this->fenceValue, this->fenceEvent);
        for (int i = 0; i < this->nFrames; ++i) {
            // Any references to the back buffers must be released
            //  before the swap chain can be resized.
            this->backBuffers[i].Reset();
            this->frameFenceValues[i] = this->frameFenceValues[this->currentBackBufferIndex];
        }
        for (int i = 0; i < this->nFrames; ++i) {
            // Any references to the back buffers must be released
            //  before the swap chain can be resized.
            this->backBuffers[i].Reset();
            this->frameFenceValues[i] = this->frameFenceValues[this->currentBackBufferIndex];
        }
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(this->swapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(this->swapChain->ResizeBuffers(this->nFrames, this->clientWidth, this->clientHeight,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews(this->device, this->swapChain, this->rTVDescriptorHeap);
    }
}

void Application::SetFullscreen(bool fullscreen)
{
    if (this->fullscreen != fullscreen) {
        this->fullscreen = fullscreen;

        if (this->fullscreen) {
            // Store the current window dimensions so they can be restored 
            //  when switching out of fullscreen state.
            ::GetWindowRect(this->hWnd, &this->windowRect);
            // Set the window style to a borderless window so the client area fills
            //  the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(this->hWnd, GWL_STYLE, windowStyle);
            // Query the name of the nearest display device for the window.
            //  This is required to set the fullscreen dimensions of the window
            //  when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(this->hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);
            ::SetWindowPos(this->hWnd, HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(this->hWnd, SW_MAXIMIZE);
        } else {
            // Restore all the window decorators.
            ::SetWindowLong(this->hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(this->hWnd, HWND_NOTOPMOST,
                this->windowRect.left,
                this->windowRect.top,
                this->windowRect.right - this->windowRect.left,
                this->windowRect.bottom - this->windowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(this->hWnd, SW_NORMAL);
        }
    }
}

void Application::finish()
{
    this->Flush(this->commandQueue, this->fence, this->fenceValue, this->fenceEvent);
    ::CloseHandle(this->fenceEvent);
}
