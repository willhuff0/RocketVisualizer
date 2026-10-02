#pragma once

#include <raylib.h>

class OrbitCamera {
public:
    OrbitCamera();

    void Tick(Vector3 target);

    const Camera& GetRaylibCamera() const;

private:
    Camera _camera = {};

    float _distance = 10.0f;
    float _angleX = 0.785398f;
    float _angleY = 0.523599f;
};
