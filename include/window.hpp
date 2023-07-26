#pragma once

#include <gainput/gainput.h>
#include <application.hpp>

class Window {
   public:
    ComPtr<ID3D12Device2> device;
    HWND hWnd;
    RECT windowRect;
    bool tearingSupported;
    Application* app = nullptr;
    uint32_t width, height;
    std::unique_ptr<gainput::InputManager> inputManager = nullptr;
    std::unique_ptr<gainput::InputMap> inputMap = nullptr;
    gainput::DeviceId keyboardID, mouseID, rawMouseID;

    void registerApp(Application* app);
    void initialize(HINSTANCE hInstance,
        const std::string& title,
        uint32_t width,
        uint32_t height);

    static Window* get() {
        static Window window{};
        return &window;
    }

   private:
    explicit Window() = default;
};