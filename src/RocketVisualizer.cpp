#include <RocketVisualizer.hpp>

#include "CsvTimelineLoader.hpp"

#include <raymath.h>
#include <rlgl.h>
#include <rlImGui.h>
#include <imgui.h>

#include <iomanip>
#include <algorithm>
#include <cmath>

const double RAD_TO_DEG = 57.295779513082320876798154814105f;
const double DEG_TO_RAD = 1.0 / 57.295779513082320876798154814105f;

const double ARROW_KEY_SEEK_STEP_SECONDS = 0.01f;
const double CTRL_ARROW_KEY_SEEK_STEP_SECONDS = 0.1f;

static void setupWindow() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Rocket Visualizer");

    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);

    int windowWidth = static_cast<int>(static_cast<float>(monitorWidth) * 0.75f);
    int windowHeight = static_cast<int>(static_cast<float>(monitorHeight) * 0.75f);

    Vector2 monitorPos = GetMonitorPosition(monitor);
    int monitorPosX = static_cast<int>(monitorPos.x);
    int monitorPosY = static_cast<int>(monitorPos.y);

    int x = monitorPosX + monitorWidth / 2 - windowWidth / 2;
    int y = monitorPosY + monitorHeight / 2 - windowHeight / 2;

    BeginDrawing();
    EndDrawing();

    SetWindowSize(windowWidth, windowHeight);
    SetWindowPosition(x, y);
    SetExitKey(KEY_NULL);
}

static std::string formatDouble(double value) {
    const int width = 11;
    const int precision = 6;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << std::setw(width) << value;
    return oss.str();
}

static std::string formatTime(double time) {
    int currentMin = static_cast<int>(time) / 60;
    int currentSec = static_cast<int>(time) % 60;
    std::ostringstream oss;
    oss << std::fixed << std::setfill('0') << std::setw(2) << currentMin << ":";
    oss << std::fixed << std::setfill('0') << std::setw(2) << currentSec;
    return oss.str();
}

static void drawXYGrid(int slices, float spacing) {
    const float halfSize = static_cast<float>(slices) * spacing * 0.5f;
    const Color color = Fade(LIGHTGRAY, 0.35f);

    for (int i = 0; i <= slices; ++i) {
        const float offset = -halfSize + static_cast<float>(i) * spacing;

        DrawLine3D(
            Vector3{offset, -halfSize, 0.0f},
            Vector3{offset, halfSize, 0.0f},
            color
        );
        DrawLine3D(
            Vector3{-halfSize, offset, 0.0f},
            Vector3{halfSize, offset, 0.0f},
            color
        );
    }
}

static void drawAxisLines(Vector3 position, Quaternion rotation, float length, bool includeUp = true, float alpha = 1.0) {
    Matrix matRotation = QuaternionToMatrix(rotation);

    Vector3 localX = { matRotation.m0, matRotation.m1, matRotation.m2 };
    Vector3 localY = { matRotation.m4, matRotation.m5, matRotation.m6 };
    Vector3 localZ = { matRotation.m8, matRotation.m9, matRotation.m10 };

    Vector3 endX = Vector3Add(position, Vector3Scale(localX, length));
    Vector3 endY = Vector3Add(position, Vector3Scale(localY, length));
    Vector3 endZ = Vector3Add(position, Vector3Scale(localZ, length));

    DrawLine3D(position, endX, ColorAlpha(RED, alpha));
    DrawLine3D(position, endY, ColorAlpha(GREEN, alpha));
    if (includeUp) DrawLine3D(position, endZ, ColorAlpha(BLUE, alpha));
}

RocketVisualizer::RocketVisualizer()
    : _csvLoader([&](RocketConfig newConfig, const auto& newTimeline) {
        SetConfigAndTimeline(newConfig, newTimeline);
    })
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    setupWindow();
    rlImGuiSetup(true);

    auto& io = ImGui::GetIO();
    io.FontDefault = io.Fonts->AddFontFromFileTTF("../resources/RobotoMono.ttf", 18.0f);

    _bodyModel = LoadModel("../resources/Rocket_Body.glb");
    _finModels["Fin_Top"] = LoadModel("../resources/Rocket_Fin_Top.glb");
    _finModels["Fin_Bottom"] = LoadModel("../resources/Rocket_Fin_Bottom.glb");
    _finModels["Fin_Right"] = LoadModel("../resources/Rocket_Fin_Right.glb");
    _finModels["Fin_Left"] = LoadModel("../resources/Rocket_Fin_Left.glb");

    if (_timeline != nullptr && _timeline->IsCapableOfLivePlayback()) {
        _playback.FollowLive();
    } else {
        _playback.Play();
    }
}

RocketVisualizer::~RocketVisualizer() {
    rlImGuiShutdown();
    CloseWindow();
}

void RocketVisualizer::SetConfigAndTimeline(RocketConfig config, std::shared_ptr<IRocketTimeline> timeline) {
    _config = std::move(config);
    _timeline = std::move(timeline);
    if (_timeline == nullptr) return;

    auto maybeRange = _timeline->GetRange();
    if (maybeRange.has_value()) {
        _playback.Seek(maybeRange.value().begin);
    }

    if (_timeline->IsCapableOfLivePlayback()) {
        _playback.FollowLive();
    } else {
        _playback.Pause();
    }
}

void RocketVisualizer::Run() {
    _mainLoop();
}

void RocketVisualizer::_mainLoop() {
    while (!WindowShouldClose() && _running) {
        _tick();
        _draw();
    }
}

void RocketVisualizer::_tick() {
    rlImGuiBegin();

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) {
        _csvLoader.ShowImportDialog();
    }

    if (_timeline == nullptr) return;

    auto maybeRange = _timeline->GetRange();
    if (!maybeRange.has_value()) return;
    auto range = maybeRange.value();

    if (IsKeyPressed(KEY_SPACE)) {
        if (_playback.GetPlaybackMode() != PlaybackMode::Paused) {
            _playback.Pause();
        } else {
            _playback.Play();
            if (_playback.GetCursor() >= range.end - 0.001) {
                _playback.Seek(range.begin);
            }
        }
    }
    auto seekSeconds = IsKeyDown(KEY_LEFT_CONTROL) ? CTRL_ARROW_KEY_SEEK_STEP_SECONDS : ARROW_KEY_SEEK_STEP_SECONDS;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        _playback.Seek(std::clamp(_playback.GetCursor() - seekSeconds, range.begin, range.end));
        if (_playback.GetCursor() <= range.begin + 0.001) {
            _playback.Pause();
            _playback.Seek(range.begin);
        } else if (_playback.GetPlaybackMode() == PlaybackMode::FollowLive) {
            _playback.Play();
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        _playback.Seek(std::clamp(_playback.GetCursor() + seekSeconds, range.begin, range.end));
        if (_timeline->IsCapableOfLivePlayback() && _playback.GetCursor() >= range.end - 0.001) {
            _playback.FollowLive();
        }
    }
    if (_timeline->IsCapableOfLivePlayback() && IsKeyPressed(KEY_END)) {
        _playback.FollowLive();
    }

    _playback.Update(GetFrameTime(), range);
    if (!_timeline->IsCapableOfLivePlayback() && _playback.GetCursor() >= range.end - 0.001) {
        _playback.Pause();
        _playback.Seek(range.end);
    }

    auto maybeState = _timeline->GetStateAt(_playback.GetCursor(), _interpolate);
    if (!maybeState.has_value()) return;
    auto state = maybeState.value();

    // TODO: move world instead of rocket and camera
    _state = state;
    _position = {
        static_cast<float>(state.position.x()),
        static_cast<float>(state.position.y()),
        static_cast<float>(state.position.z())
    };
    _rotation = {
        static_cast<float>(state.orientation.x()),
        static_cast<float>(state.orientation.y()),
        static_cast<float>(state.orientation.z()),
        static_cast<float>(state.orientation.w())
    };

    _updateRocketTransform();

    _camera.Tick(_position);
}

void RocketVisualizer::_updateRocketTransform() {
    Matrix translationMatrix = MatrixTranslate(_position.x, _position.y, _position.z);
    Matrix rotationMatrix = QuaternionToMatrix(_rotation);
    Matrix scaleMatrix = MatrixScale(0.1f, 0.1f, 0.1f);

    Matrix modelMatrix = MatrixMultiply(scaleMatrix, MatrixMultiply(rotationMatrix, translationMatrix));
    _bodyModel.transform = modelMatrix;

    for (auto& [_, finModel] : _finModels) {
        finModel.transform = modelMatrix;
    }
    for (size_t j = 0; j < MAX_FIN_COUNT; j++) {
        auto& nodeName = _config.finNodeNames[j];
        if (nodeName.empty()) break;

        auto finModelIter = _finModels.find(nodeName);
        if (finModelIter == _finModels.end()) continue;
        auto& finModel = finModelIter->second;

        auto angle = _state.actualFinAngles[j] * DEG_TO_RAD;

        auto rotationAxisEigen = _config.finRotationAxes[j];
        Vector3 rotationAxis = {
            static_cast<float>(rotationAxisEigen.x()),
            static_cast<float>(rotationAxisEigen.y()),
            static_cast<float>(rotationAxisEigen.z())
        };
        Matrix finRotationMatrix = MatrixRotate(rotationAxis, static_cast<float>(angle));

        finModel.transform = MatrixMultiply(finRotationMatrix, modelMatrix);
    }
}

void RocketVisualizer::_draw() {
    BeginDrawing();

        ClearBackground(Color{20, 20, 20, 255});

        BeginMode3D(_camera.GetRaylibCamera());

            DrawModel(_bodyModel, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            for (const auto & [_, finModel] : _finModels) {
                DrawModel(finModel, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            }
            drawXYGrid(10, 1.0f);

            rlDrawRenderBatchActive();
            rlDisableDepthTest();
            drawAxisLines(_position, _rotation, 3.0f);
            rlDrawRenderBatchActive();
            rlEnableDepthTest();

            drawAxisLines(Vector3{5.0f, 5.0f, 0.001f}, Quaternion{0.0f, 0.0f, 0.0f, 1.0f}, 1.0f, false, 1.0f);

        EndMode3D();

    _drawUI();

    rlImGuiEnd();
    EndDrawing();
}

static bool ImGuiSliderDoubleWithSteps(const char* label, double* v, double v_min, double v_max, double v_step, const char* display_format) {
    if (!display_format)
        display_format = "%.3f";

    char text_buf[64] = {};
    std::snprintf(text_buf, sizeof(text_buf), display_format, *v);

    // Map from [v_min,v_max] to [0,N]
    const int countValues = int((v_max-v_min)/v_step);
    int v_i = int((*v - v_min)/v_step);
    const bool value_changed = ImGui::SliderInt(label, &v_i, 0, countValues, text_buf);

    // Remap from [0,N] to [v_min,v_max]
    *v = v_min + double(v_i) * v_step;
    return value_changed;
}

void RocketVisualizer::_drawUI() {
    _csvLoader.DrawUI();
    _drawMenuBarUI();
    _drawRocketStateUI();
    _drawFinAnglesStateUI();

    if (_timeline != nullptr) {
        auto maybeRange = _timeline->GetRange();
        if (maybeRange.has_value()) {
            auto range = maybeRange.value();
            auto cursor = _playback.GetCursor();

            auto beginTimeString = formatTime(range.begin);
            auto endTimeString = formatTime(range.end);

            std::string currentTimeString;
            if (_playback.GetPlaybackMode() == PlaybackMode::FollowLive) {
                currentTimeString = "Live";
            } else {
                currentTimeString = formatTime(cursor);
            }

            const float timelineWindowHeight = 68.0f;
            const float timelineWindowWidthFrac = 0.6f;

            ImGuiViewport* viewport = ImGui::GetMainViewport();
            float timelineWindowWidth = viewport->WorkSize.x * timelineWindowWidthFrac;

            float timelineWindowX = viewport->WorkPos.x + (viewport->WorkSize.x - timelineWindowWidth) * 0.5f;
            float timelineWindowY = viewport->WorkPos.y + viewport->WorkSize.y - timelineWindowHeight;

            ImGui::SetNextWindowPos(ImVec2(timelineWindowX, timelineWindowY));
            ImGui::SetNextWindowSize(ImVec2(timelineWindowWidth, timelineWindowHeight));
            ImGui::Begin("##TimelineWindow", nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings);

            ImGui::Text(beginTimeString.c_str());
            ImGui::SameLine();

            bool isGrabHidden = false;
            if (_playback.GetPlaybackMode() == PlaybackMode::FollowLive) {
                ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
                isGrabHidden = true;
            }
            float textAfterWidth = ImGui::CalcTextSize(endTimeString.c_str()).x;
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float sliderWidth = ImGui::GetContentRegionAvail().x - textAfterWidth - spacing;
            ImGui::SetNextItemWidth(sliderWidth);
            if (ImGui::SliderScalar("##TimelineSlider", ImGuiDataType_Double, &cursor, &range.begin, &range.end, currentTimeString.c_str(), ImGuiSliderFlags_NoInput)) {
                if (cursor <= range.begin + 0.001) {
                    _playback.Pause();
                    _playback.Seek(range.begin);
                } else if (_timeline->IsCapableOfLivePlayback() && cursor >= range.end - 0.001) {
                    _playback.FollowLive();
                } else {
                    if (_playback.GetPlaybackMode() == PlaybackMode::FollowLive) {
                        _playback.Play();
                    }
                    _playback.Seek(cursor);
                }
            }
            if (isGrabHidden) {
                ImGui::PopStyleColor(2);
            }

            ImGui::SameLine();
            ImGui::Text(endTimeString.c_str());

            const float playPauseButtonWidth = 60.0f;
            float textBeforeWidth = ImGui::CalcTextSize(beginTimeString.c_str()).x;
            float playPauseButtonX = textBeforeWidth + spacing * 2.0f;
            ImGui::SetCursorPosX(playPauseButtonX);
            if (_playback.GetPlaybackMode() != PlaybackMode::Paused) {
                if (ImGui::Button("Pause", ImVec2(playPauseButtonWidth, 0.0f))) {
                    _playback.Pause();
                }
            } else {
                if (ImGui::Button("Play", ImVec2(playPauseButtonWidth, 0.0f))) {
                    _playback.Play();
                    if (_playback.GetCursor() >= range.end - 0.001) {
                        _playback.Seek(range.begin);
                    }
                }
            }

            ImGui::End();
        }
    }
}

void RocketVisualizer::_drawMenuBarUI() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open CSV", "Ctrl+O")) {
                _csvLoader.ShowImportDialog();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                _running = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings")) {
            ImGui::MenuItem("Interpolate", nullptr, &_interpolate);

            ImGui::Separator();
            ImGui::SetNextItemWidth(150.0f);
            double speed = _playback.GetSpeed();
            ImGuiSliderDoubleWithSteps("Speed", &speed, 0.0, 2.0, 0.1, "%.1f");
            _playback.SetSpeed(speed);

            ImGui::EndMenu();
        }

        if (_timeline != nullptr) {
            auto maybeRange = _timeline->GetRange();
            if (maybeRange.has_value()) {
                auto range = maybeRange.value();

                if (ImGui::BeginMenu("Timeline")) {
                    if (_timeline->IsCapableOfLivePlayback() && _playback.GetPlaybackMode() != PlaybackMode::FollowLive) {
                        if (ImGui::MenuItem("Go To Live", "End")) {
                            _playback.FollowLive();
                        }

                        ImGui::Separator();
                    }

                    if (_playback.GetPlaybackMode() != PlaybackMode::Paused) {
                        if (ImGui::MenuItem("Pause", "Space")) {
                            _playback.Pause();
                        }
                    } else {
                        if (ImGui::MenuItem("Play", "Space")) {
                            _playback.Play();
                            if (_playback.GetCursor() >= range.end - 0.001) {
                                _playback.Seek(range.begin);
                            }
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Forward 0.01s", "Right Arrow")) {
                        _playback.Seek(std::clamp(_playback.GetCursor() + ARROW_KEY_SEEK_STEP_SECONDS, range.begin, range.end));
                        if (_timeline->IsCapableOfLivePlayback() && _playback.GetCursor() >= range.end - 0.001) {
                            _playback.FollowLive();
                        }
                    }
                    if (ImGui::MenuItem("Backward 0.01s", "Left Arrow")) {
                        _playback.Seek(std::clamp(_playback.GetCursor() - ARROW_KEY_SEEK_STEP_SECONDS, range.begin, range.end));
                        if (_playback.GetCursor() <= range.begin + 0.001) {
                            _playback.Pause();
                            _playback.Seek(range.begin);
                        } else if (_playback.GetPlaybackMode() == PlaybackMode::FollowLive) {
                            _playback.Play();
                        }
                    }

                    ImGui::EndMenu();
                }
            }
        }

        ImGui::EndMainMenuBar();
    }
}

void RocketVisualizer::_drawRocketStateUI() {
    if (_timeline != nullptr) {
        if (ImGui::Begin("Rocket State", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            constexpr auto timePrefix =        "Time              : ";
            constexpr auto positionPrefix =    "Position   (XYZ)  : ";
            constexpr auto quaternionPrefix =  "Quaternion (XYZW) : ";
            constexpr auto eulerAnglesPrefix = "Euler      (ZYX)  : ";

            auto timeString = timePrefix +
                formatDouble(_playback.GetCursor());

            auto positionString = positionPrefix +
                formatDouble(_state.position.x()) + ", " +
                formatDouble(_state.position.y()) + ", " +
                formatDouble(_state.position.z());

            auto quaternionString = quaternionPrefix +
                formatDouble(_state.orientation.x()) + ", " +
                formatDouble(_state.orientation.y()) + ", " +
                formatDouble(_state.orientation.z()) + ", " +
                formatDouble(_state.orientation.w());

            Eigen::Matrix3d rotationMatrix = _state.orientation.toRotationMatrix();
            Eigen::Vector3d eulerAnglesZYX = rotationMatrix.canonicalEulerAngles(2, 1, 0);
            auto eulerAnglesString = eulerAnglesPrefix +
                formatDouble(eulerAnglesZYX.x() * RAD_TO_DEG) + ", " +
                formatDouble(eulerAnglesZYX.y() * RAD_TO_DEG) + ", " +
                formatDouble(eulerAnglesZYX.z() * RAD_TO_DEG);

            ImGui::Text(timeString.c_str());
            ImGui::Spacing();
            ImGui::Text(positionString.c_str());
            ImGui::Text(quaternionString.c_str());
            ImGui::Text(eulerAnglesString.c_str());
        }
        ImGui::End();
    }
}

void RocketVisualizer::_drawFinAnglesStateUI() {
    if (_timeline != nullptr && !_config.finNodeNames[0].empty()) {
        if (ImGui::Begin("Fin States", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("Node              : Actual Angle, Target Angle");
            ImGui::Spacing();

            for (size_t j = 0; j < MAX_FIN_COUNT; j++) {
                if (_config.finNodeNames[j].empty()) break;

                std::ostringstream oss;
                oss << std::left << std::fixed << std::setw(17) << _config.finNodeNames[j];
                auto prefixStr = oss.str() + " : ";

                auto str = prefixStr +
                    formatDouble(_state.actualFinAngles[j]) + ", " +
                    formatDouble(_state.targetFinAngles[j]);

                ImGui::Text(str.c_str());
            }
        }
        ImGui::End();
    }
}
