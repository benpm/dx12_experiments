#pragma once

#include <Windows.h>

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

#include <shellapi.h>
#include <wrl.h>
#include <cassert>
#include <chrono>
#include <string>

#include <logging.hpp>

using namespace Microsoft::WRL;
using namespace DirectX;

inline void chkDX(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::exception();
    }
}