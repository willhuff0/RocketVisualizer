#pragma once

#include "VectorRocketTimeline.hpp"
#include "CoordinateSystemImportUI.hpp"

#include <imgui.h>

#include <memory>
#include <string>
#include <functional>

struct CsvQuaternionConfig {
    std::string wName = "qw";
    std::string xName = "qx";
    std::string yName = "qy";
    std::string zName = "qz";
};

struct CsvPositionConfig {
    std::string xName = "px";
    std::string yName = "py";
    std::string zName = "pz";
};

struct CsvFinConfig {
    std::string actualAngleName = "";
    std::string targetAngleName = "";

    std::string nodeName = "";
    Eigen::Vector3d rotationAxis = {0.0, 1.0, 0.0};
};

struct CsvImportConfig {
    std::string configName;

    CoordinateSystemImportConfig coordinateSystem;

    std::string timeName = "time";
    CsvQuaternionConfig quaternion;
    CsvPositionConfig position;
    CsvFinConfig fins[MAX_FIN_COUNT] = {};
    size_t finCount = 0;
};

class CsvTimelineLoader {
public:
    explicit CsvTimelineLoader(std::function<void(RocketConfig, std::shared_ptr<VectorRocketTimeline>)> onImport);

    void ShowImportDialog();
    void DrawUI();

private:
    std::function<void(RocketConfig, std::shared_ptr<VectorRocketTimeline>)> _onImport;
    ImGuiID _popupId = 0;

    std::string _path;
    CsvImportConfig _config;

    void _drawFinList();
};
