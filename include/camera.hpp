#pragma once

#include <common.hpp>

class Camera {
   public:
    float fov = 45.0_deg;
    float aspectRatio = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    vec3 pos = {0.0f, 0.0f, 0.0f};

    virtual mat4 proj() const;
    virtual mat4 view() const;
};

class OrbitCamera : public Camera {
   public:
    float yaw = 0.0_deg;
    float pitch = 0.0_deg;
    float radius = 5.0f;

    mat4 view() const override;
};