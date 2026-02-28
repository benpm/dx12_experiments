module;

#include <DirectXMath.h>
#include <cmath>

module camera;

mat4 Camera::proj() const
{
    return XMMatrixPerspectiveFovLH(this->fov, this->aspectRatio, this->nearPlane, this->farPlane);
}

mat4 Camera::view() const
{
    return XMMatrixTranslationFromVector(-this->pos);
}

mat4 OrbitCamera::view() const
{
    return XMMatrixLookAtLH(
        XMVectorSet(
            this->radius * cos(this->pitch) * cos(this->yaw), this->radius * sin(this->pitch),
            this->radius * cos(this->pitch) * sin(this->yaw), 0.0f
        ),
        XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
    );
}