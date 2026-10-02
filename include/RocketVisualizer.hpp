#pragma once

#include "OrbitCamera.hpp"
#include "IRocketTimeline.hpp"
#include "PlaybackController.hpp"
#include "CsvTimelineLoader.hpp"

#include <raylib.h>
#include <Eigen/Dense>

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>

class RocketVisualizer {
public:
    explicit RocketVisualizer();
    ~RocketVisualizer();

    void SetConfigAndTimeline(RocketConfig config, std::shared_ptr<IRocketTimeline> timeline);

    void Run(std::function<void()> pollLiveInput = {});

private:
    OrbitCamera _camera;
    Model _bodyModel = {};
    std::unordered_map<std::string, Model> _finModels;

    std::shared_ptr<IRocketTimeline> _timeline;
    RocketConfig _config;
    PlaybackController _playback;

    CsvTimelineLoader _csvLoader;

    RocketState _state;
    Vector3 _position = {0.0, 0.0, 0.0};
    Quaternion _rotation = {0.0, 0.0, 0.0, 1.0};

    bool _interpolate = true;

    bool _running = true;

    void _mainLoop(const std::function<void()>& pollLiveInput);

    void _tick();
    void _updateRocketTransform();

    void _draw();

    void _drawUI();

    void _drawMenuBarUI();
    void _drawRocketStateUI();
    void _drawFinAnglesStateUI();
};
