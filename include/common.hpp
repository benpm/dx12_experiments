#pragma once

#include <Windows.h>

#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch"
#endif
#include "d3dx12.h"
#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#include <shellapi.h>
#include <wrl.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <string>

#include <logging.hpp>

using namespace Microsoft::WRL;
using namespace DirectX;

inline void chkDX(HRESULT hr)
{
    if (FAILED(hr)) {
        spdlog::error("chkDX failed with HRESULT: {:#010x}", static_cast<uint32_t>(hr));
        throw std::exception();
    }
}

constexpr float pi = XM_PI;
constexpr float pi2 = XM_PIDIV2;
constexpr float pi4 = XM_PIDIV4;
constexpr float tau = XM_2PI;

template <uint8_t D> struct vec
{
    static_assert(D >= 2 && D <= 4, "Invalid vector dimension");

    XMVECTOR data = XMVectorZero();

    vec() = default;
    vec(const XMVECTOR& data) : data(data) {}
    template <uint8_t _D> vec(const vec<_D>& other) : data(other.data) {}

    template <uint8_t _D> vec<D> operator+(const vec<_D>& other) const
    {
        return { XMVectorAdd(data, other.data) };
    }
    template <uint8_t _D> vec<D> operator-(const vec<_D>& other) const
    {
        return { XMVectorSubtract(data, other.data) };
    }
    template <uint8_t _D> vec<D> operator*(const vec<_D>& other) const
    {
        return { XMVectorMultiply(data, other.data) };
    }
    template <uint8_t _D> vec<D> operator/(const vec<_D>& other) const
    {
        return { XMVectorDivide(data, other.data) };
    }
    // Copy-assign XMVECTOR
    vec<D>& operator=(const XMVECTOR& other)
    {
        data = other;
        return *this;
    }

    // Conversion operator to XMVECTOR
    operator XMVECTOR() const { return data; }
};

struct vec2 : public vec<2>
{
    vec2() = default;
    vec2(const XMVECTOR& data) : vec<2>(data) {}
    vec2(float x, float y) : vec<2>(XMVectorSet(x, y, 0.0f, 0.0f)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline void x(float v) { data = XMVectorSetX(data, v); }
    inline float y() const { return XMVectorGetY(data); }
    inline void y(float v) { data = XMVectorSetY(data, v); }

    vec2& operator=(const XMVECTOR& other) { return static_cast<vec2&>(vec<2>::operator=(other)); }
};

struct vec3 : public vec<3>
{
    vec3() = default;
    vec3(const XMVECTOR& data) : vec<3>(data) {}
    vec3(float x, float y, float z = 0.0f) : vec<3>(XMVectorSet(x, y, z, 0.0f)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline void x(float v) { data = XMVectorSetX(data, v); }
    inline float y() const { return XMVectorGetY(data); }
    inline void y(float v) { data = XMVectorSetY(data, v); }
    inline float z() const { return XMVectorGetZ(data); }
    inline void z(float v) { data = XMVectorSetZ(data, v); }

    vec3& operator=(const XMVECTOR& other) { return static_cast<vec3&>(vec<3>::operator=(other)); }
};

struct vec4 : public vec<4>
{
    vec4() = default;
    vec4(const XMVECTOR& data) : vec<4>(data) {}
    vec4(float x, float y, float z = 0.0f, float w = 0.0f) : vec<4>(XMVectorSet(x, y, z, w)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline void x(float v) { data = XMVectorSetX(data, v); }
    inline float y() const { return XMVectorGetY(data); }
    inline void y(float v) { data = XMVectorSetY(data, v); }
    inline float z() const { return XMVectorGetZ(data); }
    inline void z(float v) { data = XMVectorSetZ(data, v); }
    inline float w() const { return XMVectorGetW(data); }
    inline void w(float v) { data = XMVectorSetW(data, v); }

    vec4& operator=(const XMVECTOR& other) { return static_cast<vec4&>(vec<4>::operator=(other)); }
};

struct mat4
{
    XMMATRIX data = XMMatrixIdentity();

    mat4() = default;
    mat4(const XMMATRIX& data) : data(data) {}

    mat4 operator*(const mat4& other) const { return { XMMatrixMultiply(data, other.data) }; }
    mat4& operator*=(const mat4& other)
    {
        data = XMMatrixMultiply(data, other.data);
        return *this;
    }

    mat4& operator=(const XMMATRIX& other)
    {
        data = other;
        return *this;
    }

    operator XMMATRIX() const { return data; }
};

// Vector-matrix multiply
inline vec4 operator*(const vec4& v, const mat4& m)
{
    return { XMVector4Transform(v, m) };
}

// Degrees to radians literal
inline constexpr float operator""_deg(long double degrees)
{
    return static_cast<float>(degrees) * pi / 180.0f;
}
inline constexpr float operator""_deg(unsigned long long degrees)
{
    return static_cast<float>(degrees) * pi / 180.0f;
}

// Kilobytes to bytes
inline constexpr size_t operator""_KB(unsigned long long v)
{
    return static_cast<size_t>(v * 1024uLL);
}

// Megabytes to bytes
inline constexpr size_t operator""_MB(unsigned long long v)
{
    return static_cast<size_t>(v * 1024uLL * 1024uLL);
}

// Returns the given size aligned to the given power-of-2 alignment
inline constexpr size_t align(const size_t size, const size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0 && "Alignment must be a power of 2");
    return (size + alignment - 1) & ~(alignment - 1);
}