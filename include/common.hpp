#pragma once

#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <chrono>
#include <string>
#include <cassert>
#include <wrl.h>
#include <shellapi.h>

using namespace Microsoft::WRL;
using namespace DirectX;

inline void chkDX(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}