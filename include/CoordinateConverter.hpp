#pragma once

#include <Eigen/Dense>

#include <vector>

enum class WorldFrame : int {
    NWU = 0, // +X North, +Y West, +Z Up
    SEU = 1, // +X South, +Y East, +Z Up (flight CSV)
    ENU = 2, // +X East,  +Y North, +Z Up (OpenRocket SimulationStatus)
};
constexpr WorldFrame AllWorldFrames[] = {
    WorldFrame::NWU,
    WorldFrame::SEU,
    WorldFrame::ENU,
};

enum class BodyFrame : int {
    RDF = 0, // +X Right,    +Y Down,  +Z Forward (visualizer canonical)
    BRU = 1, // +X Backward, +Y Right, +Z Up (flight CSV)
    OpenRocket = 2, // +X/+Y transverse reference axes, +Z Forward/Nose
};
constexpr BodyFrame AllBodyFrames[] = {
    BodyFrame::RDF,
    BodyFrame::BRU,
    BodyFrame::OpenRocket,
};

enum class RotationDirection : int {
    BodyToWorld = 0,
    WorldToBody = 1,
};
constexpr RotationDirection AllRotationDirections[] = {
    RotationDirection::BodyToWorld,
    RotationDirection::WorldToBody,
};

const char* GetWorldFrameLabel(WorldFrame worldFrame);
const char* GetBodyFrameLabel(BodyFrame bodyFrame);
const char* GetRotationDirectionLabel(RotationDirection rotationDirection);

std::vector<const char*> GetWorldFrameLabels();
std::vector<const char*> GetBodyFrameLabels();
std::vector<const char*> GetRotationDirectionLabels();

namespace CoordinateConverter {
    Eigen::Quaterniond ConvertOrientation(
        const Eigen::Quaterniond& inputQuaternion,
        WorldFrame inputWorld,
        BodyFrame inputBody,
        WorldFrame outputWorld,
        BodyFrame outputBody,
        RotationDirection direction);

    Eigen::Quaterniond ImportQuaternion(
        const Eigen::Quaterniond& q,
        WorldFrame csvWorld,
        BodyFrame csvBody,
        RotationDirection direction);

    Eigen::Vector3d ConvertWorldVector(
        const Eigen::Vector3d& v,
        WorldFrame inputFrame,
        WorldFrame outputFrame);

    Eigen::Vector3d ImportWorldVector(
        const Eigen::Vector3d& v,
        WorldFrame csvWorld);
}
