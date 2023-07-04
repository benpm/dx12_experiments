#include <application.hpp>
#include <window.hpp>

constexpr VertexPosColor cubeVerts[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};
constexpr WORD cubeIndices[36] = {
    0, 1, 2, 0, 2, 3, // -z
    4, 6, 5, 4, 7, 6, // +z
    4, 5, 1, 4, 1, 0, // -x
    3, 2, 6, 3, 6, 7, // +x
    1, 5, 6, 1, 6, 2, // +y
    4, 0, 3, 4, 3, 7  // -y
};

Application::Application()
{
    Window::get()->registerApp(this);

    if (!XMVerifyCPUSupport()) {
        MessageBoxA(nullptr, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }

    this->viewport = CD3DX12_VIEWPORT(
        0.0f, 0.0f, static_cast<float>(this->clientWidth), static_cast<float>(this->clientHeight));

    this->cmdQueue = CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    this->swapChain = this->createSwapChain(hWnd, this->cmdQueue.queue,
        this->clientWidth, this->clientHeight, this->nFrames);

    this->curBackBufIdx = this->swapChain->GetCurrentBackBufferIndex();

    this->rtvHeap = this->createDescHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, this->nFrames);
    this->rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    this->updateRenderTargetViews(device, this->swapChain, this->rtvHeap);

    this->loadContent();
    this->flush();
    this->isInitialized = true;
}

Application::~Application()
{
    this->flush();
}

void Application::transitionResource(
    ComPtr<ID3D12GraphicsCommandList2> cmdList, ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(), beforeState, afterState);
    cmdList->ResourceBarrier(1, &barrier);
}

void Application::clearRTV(ComPtr<ID3D12GraphicsCommandList2> cmdList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT clearColor[4])
{
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Application::clearDepth(ComPtr<ID3D12GraphicsCommandList2> cmdList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
    cmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void Application::updateBufferResource(
    ComPtr<ID3D12GraphicsCommandList2> cmdList, ID3D12Resource **pDestinationResource,
    ID3D12Resource **pIntermediateResource, size_t numElements, size_t elementSize,
    const void *bufferData, D3D12_RESOURCE_FLAGS flags)
{
    const size_t bufSize = numElements * elementSize;
    { // Create a committed resource for the GPU resource in a default heap
        const CD3DX12_HEAP_PROPERTIES pHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC pDesc = CD3DX12_RESOURCE_DESC::Buffer(bufSize, flags);
        chkDX(device->CreateCommittedResource(
            &pHeapProperties, D3D12_HEAP_FLAG_NONE, &pDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(pDestinationResource)));
    }
    
    if (bufferData) {
        // Create a committed resource for the upload
        const CD3DX12_HEAP_PROPERTIES pHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        const CD3DX12_RESOURCE_DESC pDesc = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
        chkDX(device->CreateCommittedResource(
            &pHeapProperties, D3D12_HEAP_FLAG_NONE, &pDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(pIntermediateResource)));
        
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        ::UpdateSubresources(cmdList.Get(), *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
}

void Application::resizeDepthBuffer(uint32_t width, uint32_t height)
{
    assert(this->contentLoaded);

    this->flush();
    width = std::max(1u, width);
    height = std::max(1u, height);

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };
    const CD3DX12_HEAP_PROPERTIES pHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_RESOURCE_DESC pDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    chkDX(device->CreateCommittedResource(
        &pHeapProperties, D3D12_HEAP_FLAG_NONE, &pDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue, IID_PPV_ARGS(&this->depthBuffer)));

    // Update depth-stencil view
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    device->CreateDepthStencilView(this->depthBuffer.Get(), &dsvDesc,
        this->dsvHeap->GetCPUDescriptorHandleForHeapStart());   
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
    this->curBackBufIdx = dxgiSwapChain4->GetCurrentBackBufferIndex();
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

    {
        // Update the model matrix
        float angle = static_cast<float>(elapsedSeconds) * XM_PIDIV4;
        XMStoreFloat4x4(&this->matModel, XMMatrixRotationY(angle));

        // Update the view matrix
        const XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f);
        const XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
        const XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMStoreFloat4x4(&this->matView, XMMatrixLookAtLH(eyePosition, focusPoint, upDirection));

        // Update the projection matrix
        const float aspectRatio = this->clientWidth / this->clientHeight;
        XMStoreFloat4x4(&this->matProj, XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectRatio, 0.1f, 100.0f));
    }
}

void Application::render()
{
    auto backBuffer = this->backBuffers[this->curBackBufIdx];
    auto cmdList = this->cmdQueue.getCmdList();

    {
        // Transition backbuffer to render target state, then clear
        this->transitionResource(cmdList, backBuffer,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(this->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            this->curBackBufIdx, this->rtvDescSize);
        this->clearRTV(cmdList, rtv, clearColor);
        auto dsv = this->dsvHeap->GetCPUDescriptorHandleForHeapStart();
        this->clearDepth(cmdList, dsv);

        // Set pipeline state and root signature
        cmdList->SetPipelineState(this->pipelineState.Get());
        cmdList->SetGraphicsRootSignature(this->rootSignature.Get());
        
        // Setup input assembler, rasterizer state
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &this->vertexBufferView);
        cmdList->IASetIndexBuffer(&this->indexBufferView);
        cmdList->RSSetViewports(1, &this->viewport);
        cmdList->RSSetScissorRects(1, &this->scissorRect);

        // Bind the render targets
        cmdList->OMSetRenderTargets(1, &rtv, true, &dsv);

        // Update root params: MVP matrix into constant buffer for vert shader
        XMMATRIX mvpMatrix = XMLoadFloat4x4(&this->matModel) *
            XMLoadFloat4x4(&this->matView) *
            XMLoadFloat4x4(&this->matProj);
        cmdList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

        // Draw
        cmdList->DrawIndexedInstanced(_countof(cubeIndices), 1, 0, 0, 0);
    }

    // Present
    {
        // Transition backbuffer to present state
        this->transitionResource(cmdList, backBuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        // Execute command list and present, then wait for completion
        this->frameFenceValues[this->curBackBufIdx] = this->cmdQueue.execCmdList(cmdList);

        UINT syncInterval = this->vsync ? 1 : 0;
        UINT presentFlags = this->tearingSupported && !this->vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
        chkDX(this->swapChain->Present(syncInterval, presentFlags));
        this->curBackBufIdx = this->swapChain->GetCurrentBackBufferIndex();

        this->cmdQueue.waitForFenceVal(this->frameFenceValues[this->curBackBufIdx]);
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
        this->cmdQueue.flush();
        for (int i = 0; i < this->nFrames; ++i) {
            // Any references to the back buffers must be released
            //  before the swap chain can be resized.
            this->backBuffers[i].Reset();
            // this->frameFenceValues[i] = this->frameFenceValues[this->currentBackBufferIndex];
        }
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        chkDX(this->swapChain->GetDesc(&swapChainDesc));
        chkDX(this->swapChain->ResizeBuffers(this->nFrames, this->clientWidth, this->clientHeight,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
        spdlog::debug("resized buffers in swap chain to ({},{})", this->clientWidth, this->clientHeight);

        this->curBackBufIdx = this->swapChain->GetCurrentBackBufferIndex();

        updateRenderTargetViews(this->device, this->swapChain, this->rtvHeap);

        this->viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
            static_cast<float>(this->clientWidth), static_cast<float>(this->clientHeight));
        this->resizeDepthBuffer(this->clientWidth, this->clientHeight);

        this->flush();
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

void Application::flush()
{
    this->cmdQueue.flush();
}

void Application::onKeyPressed(UINT key, bool alt)
{
    switch (key) {
        case 'V':
            this->vsync = !this->vsync;
            break;
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        case VK_RETURN:
            if (alt) {
                case VK_F11:
                this->setFullscreen(!this->fullscreen);
            }
            break;
    }
}

bool Application::loadContent()
{
    auto cmdList = this->cmdQueue.getCmdList();

    // Upload vertex buffer data
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    this->updateBufferResource(cmdList, &this->vertexBuffer, &intermediateVertexBuffer,
        _countof(cubeVerts), sizeof(VertexPosColor), cubeVerts);

    // Create the vertex buffer view
    this->vertexBufferView.BufferLocation = this->vertexBuffer->GetGPUVirtualAddress();
    this->vertexBufferView.SizeInBytes = sizeof(cubeVerts);
    this->vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    // Upload index buffer data
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    this->updateBufferResource(cmdList, &this->indexBuffer, &intermediateIndexBuffer,
        _countof(cubeIndices), sizeof(WORD), cubeIndices);
    
    // Create the index buffer view
    this->indexBufferView.BufferLocation = this->indexBuffer->GetGPUVirtualAddress();
    this->indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    this->indexBufferView.SizeInBytes = sizeof(cubeIndices);

    // Create the descriptor heap for the depth-stencil view
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    chkDX(this->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&this->dsvHeap)));

    // Load pre-compiled shaders
    ComPtr<ID3DBlob> vertexShader, pixelShader;
    chkDX(D3DReadFileToBlob(L"vertex_shader.cso", &vertexShader));
    chkDX(D3DReadFileToBlob(L"pixel_shader.cso", &pixelShader));

    // Create the vertex input layout which describes the way the vertex buffer is structured
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
            D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Specify a root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(this->device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }
    const D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    CD3DX12_ROOT_PARAMETER1 rootParams[1];
    rootParams[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, rootSigFlags);

    // Serialize and create the root signature
    ComPtr<ID3DBlob> rootSigBlob, errorBlob;
    chkDX(D3DX12SerializeVersionedRootSignature(&rootSigDesc, featureData.HighestVersion, &rootSigBlob, &errorBlob));
    chkDX(this->device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(&this->rootSignature)));
    
    // Create the pipeline state object
    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineStateStream.pRootSignature = this->rootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;
    D3D12_PIPELINE_STATE_STREAM_DESC psoDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };
    chkDX(this->device->CreatePipelineState(&psoDesc, IID_PPV_ARGS(&this->pipelineState)));

    // Execute the created command list on the GPU
    uint64_t fenceValue = this->cmdQueue.execCmdList(cmdList);
    this->cmdQueue.waitForFenceVal(fenceValue);

    // Resize / create the depth buffer
    this->contentLoaded = true;
    this->resizeDepthBuffer(this->clientWidth, this->clientHeight);

    return this->contentLoaded;
}
