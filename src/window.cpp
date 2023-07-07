#include <windowsx.h>
#include <input.hpp>
#include <iostream>
#include <window.hpp>

LRESULT CALLBACK WndProc(HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    Application* app = Window::get()->app;
    if (app != nullptr && app->isInitialized && app->contentLoaded) {
        switch (message) {
            case WM_PAINT:
                app->handleEvent(EventPaint{});
                break;
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
                app->handleEvent(EventKeyDown{.key = static_cast<Key>(wParam)});
                break;
            case WM_SYSKEYUP:
            case WM_KEYUP:
                app->handleEvent(EventKeyUp{.key = static_cast<Key>(wParam)});
                break;
            // The default window procedure will play a system notification
            // sound when pressing the Alt+Enter keyboard combination if this
            // message is not handled.
            case WM_SYSCHAR:
                break;
            case WM_SIZE: {
                RECT clientRect = {};
                ::GetClientRect(app->hWnd, &clientRect);
                app->handleEvent(
                    EventResize{.width = static_cast<uint32_t>(
                                    clientRect.right - clientRect.left),
                        .height = static_cast<uint32_t>(
                            clientRect.bottom - clientRect.top)});
            } break;
            case WM_DESTROY:
                ::PostQuitMessage(0);
                break;
            case WM_MOUSEMOVE: {
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);
                app->handleEvent(EventMouseMove{.x = xPos, .y = yPos});
            } break;
            default:
                return ::DefWindowProcW(hwnd, message, wParam, lParam);
        }
    } else {
        return ::DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}

void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName) {
    // Register a window class for creating our render window with.
    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, NULL);
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm = ::LoadIcon(hInst, NULL);

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

void EnableDebugLayer() {
#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    ComPtr<ID3D12Debug> debugInterface;
    chkDX(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
    std::cout << "Direct3D Debug Layer activated" << std::endl;
#endif
}

HWND MakeWindow(const wchar_t* windowClassName,
    HINSTANCE hInst,
    const wchar_t* windowTitle,
    uint32_t width,
    uint32_t height) {
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = {
        0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left
    // corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hWnd = ::CreateWindowExW(NULL, windowClassName, windowTitle,
        WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, NULL,
        NULL, hInst, nullptr);

    assert(hWnd && "Failed to create window");

    return hWnd;
}

// Query for compatible adapter (GPU device)
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp) {
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    chkDX(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;

    if (useWarp) {
        chkDX(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        chkDX(dxgiAdapter1.As(&dxgiAdapter4));
    } else {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) !=
                         DXGI_ERROR_NOT_FOUND;
             ++i) {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without
            // actually creating it. The adapter with the largest dedicated
            // video memory is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                    D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory >
                    maxDedicatedVideoMemory) {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                chkDX(dxgiAdapter1.As(&dxgiAdapter4));
            }
        }
    }

    return dxgiAdapter4;
}

// Create the D3D12 device using the given adapter
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter) {
    ComPtr<ID3D12Device2> d3d12Device2;
    chkDX(D3D12CreateDevice(
        adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device2.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  // I'm
                                                                           // really
                                                                           // not
                                                                           // sure
                                                                           // how
                                                                           // to
                                                                           // avoid
                                                                           // this
                                                                           // message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,  // This warning occurs when
                                                     // using capture frame
                                                     // while graphics
                                                     // debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,  // This warning occurs
                                                       // when using capture
                                                       // frame while graphics
                                                       // debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        // NewFilter.DenyList.NumCategories = _countof(Categories);
        // NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        chkDX(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif

    return d3d12Device2;
}

bool CheckTearingSupport() {
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
    // graphics debugging tools which will not support the 1.5 factory interface
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5))) {
            if (FAILED(factory5->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing,
                    sizeof(allowTearing)))) {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

void Window::registerApp(Application* application) {
    assert(this->app == nullptr);

    this->app = application;
    this->app->hWnd = this->hWnd;
    this->app->device = this->device;
    this->app->tearingSupported = this->tearingSupported;
    this->app->clientWidth = this->width;
    this->app->clientHeight = this->height;
}

void Window::initialize(HINSTANCE hInstance,
    const std::string& title,
    uint32_t w,
    uint32_t h) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    EnableDebugLayer();

    this->tearingSupported = CheckTearingSupport();

    const wchar_t* windowClassName = L"DX12WindowClass";
    RegisterWindowClass(hInstance, windowClassName);
    const std::wstring wTitle(title.begin(), title.end());
    this->hWnd = MakeWindow(windowClassName, hInstance, wTitle.c_str(), w, h);
    GetWindowRect(this->hWnd, &this->windowRect);

    this->device = CreateDevice(GetAdapter(false));
    this->width = w;
    this->height = h;

    ::ShowWindow(this->hWnd, SW_SHOW);
}
