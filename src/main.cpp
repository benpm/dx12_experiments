#include <input.hpp>
#include <window.hpp>

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int) {
    setupLogging();

    Window::get()->initialize(hInstance, "D3D12 Experiment", 1280, 720);
    Application app;

    // Input map just to show example for closing window with escape
    Window::get()->inputMap->MapBool(
        Button::Exit, Window::get()->keyboardID, gainput::KeyEscape);

    bool quit = false;
    while (!quit) {
        // Handle windows messages, sending to gainput
        Window::get()->inputManager->Update();
        MSG msg = {};
        while (::PeekMessage(&msg, Window::get()->hWnd, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            Window::get()->inputManager->HandleMessage(msg);
        }
        if (Window::get()->inputMap->GetBoolWasDown(Button::Exit)) {
            quit = true;
        }
    }

    return 0;
}
