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

constexpr float pi = XM_PI;
constexpr float tau = XM_2PI;

template <int D> struct vec {
    XMVECTOR data = XMVectorZero();

    vec() = default;
    vec(const XMVECTOR& data) : data(data) {}
    template <int _D> vec(const vec<_D>& other) : data(other.data) {}

    template <int _D> vec<D> operator+(const vec<_D>& other) const {
        return {XMVectorAdd(data, other.data)};
    }
    template <int _D> vec<D> operator-(const vec<_D>& other) const {
        return {XMVectorSubtract(data, other.data)};
    }
    template <int _D> vec<D> operator*(const vec<_D>& other) const {
        return {XMVectorMultiply(data, other.data)};
    }
    template <int _D> vec<D> operator/(const vec<_D>& other) const {
        return {XMVectorDivide(data, other.data)};
    }
    // Copy-assign XMVECTOR
    vec<D>& operator=(const XMVECTOR& other) {
        data = other;
        return *this;
    }

    // Conversion operator to XMVECTOR
    operator XMVECTOR() const { return data; }
};

struct vec2 : public vec<2> {
    vec2() = default;
    vec2(const XMVECTOR& data) : vec<2>(data) {}
    vec2(float x, float y) : vec<2>(XMVectorSet(x, y, 0.0f, 0.0f)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline float y() const { return XMVectorGetY(data); }

    vec2& operator=(const XMVECTOR& other) {
        return static_cast<vec2&>(vec<2>::operator=(other));
    }
};

struct vec3 : public vec<3> {
    vec3() = default;
    vec3(const XMVECTOR& data) : vec<3>(data) {}
    vec3(float x, float y, float z = 0.0f)
        : vec<3>(XMVectorSet(x, y, z, 0.0f)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline float y() const { return XMVectorGetY(data); }
    inline float z() const { return XMVectorGetZ(data); }

    vec3& operator=(const XMVECTOR& other) {
        return static_cast<vec3&>(vec<3>::operator=(other));
    }
};

struct vec4 : public vec<4> {
    vec4() = default;
    vec4(const XMVECTOR& data) : vec<4>(data) {}
    vec4(float x, float y, float z = 0.0f, float w = 0.0f)
        : vec<4>(XMVectorSet(x, y, z, w)) {}

    inline float x() const { return XMVectorGetX(data); }
    inline float y() const { return XMVectorGetY(data); }
    inline float z() const { return XMVectorGetZ(data); }
    inline float w() const { return XMVectorGetW(data); }

    vec4& operator=(const XMVECTOR& other) {
        return static_cast<vec4&>(vec<4>::operator=(other));
    }
};

struct mat4 {
    XMMATRIX data = XMMatrixIdentity();

    mat4() = default;
    mat4(const XMMATRIX& data) : data(data) {}

    mat4 operator*(const mat4& other) const {
        return {XMMatrixMultiply(data, other.data)};
    }
    mat4& operator*=(const mat4& other) {
        data = XMMatrixMultiply(data, other.data);
        return *this;
    }

    mat4& operator=(const XMMATRIX& other) {
        data = other;
        return *this;
    }

    operator XMMATRIX() const { return data; }
};

// Vector-matrix multiply
inline vec4 operator*(const vec4& v, const mat4& m) {
    return {XMVector4Transform(v, m)};
}

inline float operator""_deg(long double degrees) {
    return static_cast<float>(degrees) * pi / 180.0f;
}