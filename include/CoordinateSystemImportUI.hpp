#pragma once

#include "CoordinateConverter.hpp"

struct CoordinateSystemImportConfig {
    WorldFrame worldFrame = WorldFrame::NWU;
    BodyFrame bodyFrame = BodyFrame::RDF;
    RotationDirection rotationDirection = RotationDirection::BodyToWorld;
};

namespace CoordinateSystemImportUI {
    void Draw(CoordinateSystemImportConfig& config);
}