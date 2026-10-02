#include <OrbitCamera.hpp>

#include <raymath.h>
#include <imgui.h>

const Vector3 CAMERA_TARGET_OFFSET = {0.0f, 0.0f, 1.0f};

OrbitCamera::OrbitCamera() {
    _camera.target = CAMERA_TARGET_OFFSET;
    _camera.up = Vector3 {0.0f, 0.0f, 1.0f};
    _camera.fovy = 45.0f;
    _camera.projection = CAMERA_PERSPECTIVE;
}

void OrbitCamera::Tick(Vector3 target) {
    ImGuiIO& io = ImGui::GetIO();

    _camera.target = Vector3Add(target, CAMERA_TARGET_OFFSET);

    if (!io.WantCaptureMouse) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouseDelta = GetMouseDelta();
            _angleX -= mouseDelta.x * 0.005f;
            _angleY += mouseDelta.y * 0.005f;

            if (_angleY > 1.57f) _angleY = 1.57f;
            if (_angleY < -1.57f) _angleY = -1.57f;
        }

        _distance -= GetMouseWheelMove();
        if (_distance < 1.0f) _distance = 1.0f;
    }

    const float cosElevation = cosf(_angleY);
    _camera.position.x = _camera.target.x + _distance * cosElevation * cosf(_angleX);
    _camera.position.y = _camera.target.y + _distance * cosElevation * sinf(_angleX);
    _camera.position.z = _camera.target.z + _distance * sinf(_angleY);
}

const Camera& OrbitCamera::GetRaylibCamera() const {
    return _camera;
}
