#pragma once

#include "VectorRocketTimeline.hpp"
#include "CoordinateConverter.hpp"

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

    WorldFrame worldFrame = WorldFrame::NWU;
    BodyFrame bodyFrame = BodyFrame::RDF;
    RotationDirection rotationDirection = RotationDirection::BodyToWorld;

    std::string timeName = "time";
    CsvQuaternionConfig quaternion;
    CsvPositionConfig position;
    CsvFinConfig fins[MAX_FIN_COUNT] = {};
    size_t finCount = 0;
};

const CsvImportConfig OPEN_ROCKET_PROFILE = {
    .configName = "OpenRocket (TcpBridge)",
    .worldFrame = WorldFrame::ENU,
    .bodyFrame = BodyFrame::OpenRocket,
    .rotationDirection = RotationDirection::BodyToWorld,
    .timeName = "##0",
    .quaternion = {
        .wName = "##4",
        .xName = "##5",
        .yName = "##6",
        .zName = "##7",
    },
    .position = {
        .xName = "##1",
        .yName = "##2",
        .zName = "##3",
    },
    .fins = {
        CsvFinConfig {
            .actualAngleName = "##8",
            .targetAngleName = "##9",
            .nodeName = "Fin_Top",
            .rotationAxis = {1.0, 0.0, 0.0},
        },
        CsvFinConfig {
            .actualAngleName = "##8",
            .targetAngleName = "##9",
            .nodeName = "Fin_Bottom",
            .rotationAxis = {-1.0, 0.0, 0.0},
        },
        CsvFinConfig {
            .actualAngleName = "##8",
            .targetAngleName = "##9",
            .nodeName = "Fin_Right",
            .rotationAxis = {0.0, -1.0, 0.0},
        },
        CsvFinConfig {
            .actualAngleName = "##8",
            .targetAngleName = "##9",
            .nodeName = "Fin_Left",
            .rotationAxis = {0.0, 1.0, 0.0},
        },
    },
    .finCount = 4,
};

const CsvImportConfig TEST_CSV_PROFILE = {
    .configName = "Flight CSV",
    .worldFrame = WorldFrame::SEU,
    .bodyFrame = BodyFrame::BRU,
    .rotationDirection = RotationDirection::BodyToWorld,
    .timeName = "time_s",
    .quaternion = {
        .wName = "q0",
        .xName = "q1",
        .yName = "q2",
        .zName = "q3",
    },
    .position = {
        .xName = "",
        .yName = "",
        .zName = "p3_n_m",
    },
};

const CsvImportConfig CSV_IMPORT_PROFILES[] = {
    {
        .configName = "Visualizer (Canonical)",
    },
    OPEN_ROCKET_PROFILE,
    TEST_CSV_PROFILE
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
