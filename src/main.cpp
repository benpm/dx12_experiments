#include <window.hpp>

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    setupLogging();

    Window::get()->initialize(hInstance, "D3D12 Experiment", 1280, 720);
    Application app;
 
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return 0;
}
