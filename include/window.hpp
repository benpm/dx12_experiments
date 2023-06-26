#pragma once

#include <application.hpp>

class Window
{
public:
    ComPtr<ID3D12Device2> device;
    HWND hWnd;
    RECT windowRect;
    bool tearingSupported;
    Application* app = nullptr;

    void registerApp(Application* app);
    void initialize(HINSTANCE hInstance, const std::string& title, uint32_t width, uint32_t height);

    static Window* get() {
        static Window window{};
        return &window;
    }
private:

    explicit Window() = default;
};