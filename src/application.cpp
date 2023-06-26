#include <application.hpp>
#include <window.hpp>

Application::Application()
{
    Window::get()->registerApp(this);

    this->commandQueue = CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    this->swapChain = this->createSwapChain(hWnd, this->commandQueue.queue,
        this->clientWidth, this->clientHeight, this->nFrames);

    this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

    this->rTVDescriptorHeap = this->createDescHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, this->nFrames);
    this->rTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    this->updateRenderTargetViews(device, this->swapChain, this->rTVDescriptorHeap);
    this->commandList = this->commandQueue.getCmdList();
    
    this->isInitialized = true;
}

Application::~Application()
{
    this->finish();
}

// Create swap chain which describes the sequence of buffers used for rendering
ComPtr<IDXGISwapChain4> Application::createSwapChain(HWND hWnd, 
    ComPtr<ID3D12CommandQueue> commandQueue, 
    uint32_t width, uint32_t height, uint32_t bufferCount )
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    chkDX(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
    
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
    chkDX(dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1));

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    chkDX(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    chkDX(swapChain1.As(&dxgiSwapChain4));

    return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> Application::createDescHeap(ComPtr<ID3D12Device2> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    chkDX(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

    return descriptorHeap;
}

void Application::updateRenderTargetViews(ComPtr<ID3D12Device2> device,
    ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < this->nFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        chkDX(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        this->backBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void Application::update()
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

void Application::render()
{
    auto backBuffer = this->backBuffers[this->currentBackBufferIndex];
    this->commandList = this->commandQueue.getCmdList();

    // Clear the render target.
    {
        // Transition backbuffer to render target state
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

        uint64_t fenceVal = this->commandQueue.execCmdList(this->commandList);

        UINT syncInterval = this->vsync ? 1 : 0;
        UINT presentFlags = this->tearingSupported && !this->vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        chkDX(this->swapChain->Present(syncInterval, presentFlags));

        // Signal frame completion and wait
        this->frameFenceValues[this->currentBackBufferIndex] = fenceVal;
        this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

        this->commandQueue.waitForFenceVal(fenceVal);
    }
}

void Application::resize(uint32_t width, uint32_t height)
{
    if (this->clientWidth != width || this->clientHeight != height)
    {
        // Don't allow 0 size swap chain back buffers.
        this->clientWidth = std::max(1u, width);
        this->clientHeight = std::max(1u, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        //  are not being referenced by an in-flight command list.
        this->commandQueue.flush();
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
        chkDX(this->swapChain->GetDesc(&swapChainDesc));
        chkDX(this->swapChain->ResizeBuffers(this->nFrames, this->clientWidth, this->clientHeight,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        this->currentBackBufferIndex = this->swapChain->GetCurrentBackBufferIndex();

        updateRenderTargetViews(this->device, this->swapChain, this->rTVDescriptorHeap);
    }
}

void Application::setFullscreen(bool fullscreen)
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
    this->commandQueue.flush();
}
