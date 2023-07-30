#include <input.hpp>
#include <window.hpp>

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow) {
    setupLogging();

    Window::get()->initialize(
        hInstance, "D3D12 Experiment", 1280, 720, nCmdShow);
    Application app;

    // Input map just to show example for closing window with escape
    app.inputMap.MapBool(Button::Exit, app.keyboardID, gainput::KeyEscape);

    ::ShowWindow(Window::get()->hWnd, SW_SHOW);
    ::UpdateWindow(Window::get()->hWnd);

    MSG msg = {};
    BOOL msgReceived = TRUE;
    while (msgReceived = ::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) &&
                         msg.message != WM_QUIT) {
        if (msgReceived == -1) {
            spdlog::error("PeekMessage error: {}", ::GetLastError());
            break;
        }
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        app.inputManager.HandleMessage(msg);
        if (Window::get()->doExit) {
            spdlog::debug("exit requested");
            ::PostQuitMessage(0);
            Window::get()->doExit = false;
        }
    }
    spdlog::debug("msgReceived: {}", msgReceived);
    spdlog::debug("Window::get()->doExit: {}", Window::get()->doExit);

    return 0;
}
