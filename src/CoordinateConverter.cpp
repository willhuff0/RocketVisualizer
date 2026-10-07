#include <CoordinateConverter.hpp>

using namespace CoordinateConverter;

// The frames expected by the renderer:
constexpr WorldFrame CANONICAL_WORLD_FRAME = WorldFrame::NWU;
constexpr BodyFrame CANONICAL_BODY_FRAME = BodyFrame::RDF;

const char* GetWorldFrameLabel(WorldFrame worldFrame) {
    switch (worldFrame) {
        case WorldFrame::NWU: return "NWU [X North, Y West, Z Up] (Visualizer)";
        case WorldFrame::ENU: return "ENU [X East, Y North, Z Up] (OpenRocket)";
        case WorldFrame::NED: return "NED [X North, Y East, Z Down]";
        case WorldFrame::SEU: return "SEU [X South, Y East, Z Up]";
        default: throw std::runtime_error("Unsupported WorldFrame");
    }
}

const char* GetBodyFrameLabel(BodyFrame bodyFrame) {
    switch (bodyFrame) {
        case BodyFrame::RDF: return "RDF [X Right, Y Down, Z Forward] (Visualizer)";
        case BodyFrame::FLU: return "FLU [X Forward, Y Left, Z Up] (AHRS)";
        case BodyFrame::BRU: return "BRU [X Backward, Y Right, Z Up]";
        case BodyFrame::OpenRocket: return "OpenRocket [X/Y Transverse, Z Nose]";
        default: throw std::runtime_error("Unsupported BodyFrame");
    }
}

const char* GetRotationDirectionLabel(RotationDirection rotationDirection) {
    switch (rotationDirection) {
        case RotationDirection::BodyToWorld: return "Body -> World";
        case RotationDirection::WorldToBody: return "World -> Body";
        default: throw std::runtime_error("Unsupported RotationDirection");
    }
}

std::vector<const char*> GetWorldFrameLabels() {
    static std::vector<const char*> labels;
    if (labels.empty()) {
        for (WorldFrame worldFrame : AllWorldFrames) {
            labels.emplace_back(GetWorldFrameLabel(worldFrame));
        }
    }
    return labels;
}
std::vector<const char*> GetBodyFrameLabels() {
    static std::vector<const char*> labels;
    if (labels.empty()) {
        for (BodyFrame bodyFrame : AllBodyFrames) {
            labels.emplace_back(GetBodyFrameLabel(bodyFrame));
        }
    }
    return labels;
}
std::vector<const char*> GetRotationDirectionLabels() {
    static std::vector<const char*> labels;
    if (labels.empty()) {
        for (RotationDirection rotationDirection : AllRotationDirections) {
            labels.emplace_back(GetRotationDirectionLabel(rotationDirection));
        }
    }
    return labels;
}

static Eigen::Matrix3d getWorldBasisInNWU(WorldFrame frame) {
    // Columns are:
    //
    //   +X_frame expressed in NWU
    //   +Y_frame expressed in NWU
    //   +Z_frame expressed in NWU
    //
    // Therefore:
    //
    //   v_NWU = B * v_frame

    switch (frame) {
        case WorldFrame::NWU:
            return Eigen::Matrix3d::Identity();

        case WorldFrame::SEU: {
            // +X South = -NWU.X
            // +Y East  = -NWU.Y
            // +Z Up    = +NWU.Z
            Eigen::Matrix3d B;
            B <<
                -1,  0,  0,
                 0, -1,  0,
                 0,  0,  1;
            return B;
        }

        case WorldFrame::ENU: {
            // OpenRocket's simulation/geodetic coordinates are local ENU:
            // +X East  = -NWU.Y
            // +Y North = +NWU.X
            // +Z Up    = +NWU.Z
            Eigen::Matrix3d B;
            B <<
                 0,  1,  0,
                -1,  0,  0,
                 0,  0,  1;
            return B;
        }

        case WorldFrame::NED: {
            Eigen::Matrix3d B;
            B << 1,  0,  0,
                 0, -1,  0,
                 0,  0, -1;
            return B;
        }
    }

    throw std::runtime_error("Unsupported WorldFrame");
}

static Eigen::Matrix3d getBodyBasisInRDF(BodyFrame frame){
    // Canonical target body frame:
    //
    //   +X = Right
    //   +Y = Down
    //   +Z = Forward / Nose
    //
    // Columns are the input body's +X,+Y,+Z axes
    // expressed in canonical RDF coordinates.

    switch (frame) {
        case BodyFrame::RDF:
            return Eigen::Matrix3d::Identity();

        case BodyFrame::BRU: {
            // Input axes expressed in canonical RDF coordinates:
            //
            // Backward = ( 0,  0, -1)
            // Right    = ( 1,  0,  0)
            // Up       = ( 0, -1,  0)
            Eigen::Matrix3d B;
            B <<
                 0,  1,  0,
                 0,  0, -1,
                -1,  0,  0;
            return B;
        }

        case BodyFrame::FLU: {
            Eigen::Matrix3d B;
            B << 0, -1,  0,
                 0,  0, -1,
                 1,  0,  0;
            return B;
        }

        case BodyFrame::OpenRocket:
            // SimulationStatus.getRocketOrientationQuaternion() rotates an
            // OpenRocket reference rocket into world coordinates.  The
            // reference rocket points along +Z; +X and +Y are its two
            // transverse axes with an arbitrary but fixed roll reference.
            //
            // Both this reference and the visualizer's RDF frame are
            // right-handed with +Z through the nose, so retaining the X/Y
            // axes preserves OpenRocket's roll phase.
            return Eigen::Matrix3d::Identity();
    }

    throw std::runtime_error("Unsupported BodyFrame");
}

Eigen::Quaterniond CoordinateConverter::ConvertOrientation(
    const Eigen::Quaterniond& inputQuaternion,
    WorldFrame inputWorld,
    BodyFrame inputBody,
    WorldFrame outputWorld,
    BodyFrame outputBody,
    RotationDirection direction)
{
    Eigen::Quaterniond q = inputQuaternion.normalized();
    Eigen::Matrix3d R = q.toRotationMatrix();

    if (direction == RotationDirection::WorldToBody) R.transposeInPlace();

    const Eigen::Matrix3d B_world_in = getWorldBasisInNWU(inputWorld);
    const Eigen::Matrix3d B_world_out = getWorldBasisInNWU(outputWorld);

    const Eigen::Matrix3d B_body_in = getBodyBasisInRDF(inputBody);
    const Eigen::Matrix3d B_body_out = getBodyBasisInRDF(outputBody);

    // Coordinate transforms:
    //
    // input world -> output world
    const Eigen::Matrix3d C_world = B_world_out.transpose() * B_world_in;

    // input body -> output body
    const Eigen::Matrix3d C_body = B_body_out.transpose() * B_body_in;

    const Eigen::Matrix3d R_out = C_world * R * C_body.transpose();

    if (!R_out.allFinite()
        || !(R_out.transpose() * R_out).isApprox(Eigen::Matrix3d::Identity(), 1e-6)
        || std::abs(R_out.determinant() - 1.0) > 1e-6) {
        throw std::runtime_error("Coordinate conversion did not produce a proper rotation");
    }

    Eigen::Quaterniond result(R_out);
    result.normalize();

    return result;
}

Eigen::Quaterniond CoordinateConverter::ImportQuaternion(
    const Eigen::Quaterniond& q,
    WorldFrame csvWorld,
    BodyFrame csvBody,
    RotationDirection direction)
{
    return ConvertOrientation(
        q,
        csvWorld,
        csvBody,
        CANONICAL_WORLD_FRAME,
        CANONICAL_BODY_FRAME,
        direction);
}

Eigen::Vector3d CoordinateConverter::ConvertWorldVector(
    const Eigen::Vector3d& v,
    WorldFrame inputFrame,
    WorldFrame outputFrame)
{
    const Eigen::Matrix3d B_in = getWorldBasisInNWU(inputFrame);
    const Eigen::Matrix3d B_out = getWorldBasisInNWU(outputFrame);

    return B_out.transpose() * B_in * v;
}

Eigen::Vector3d CoordinateConverter::ImportWorldVector(
    const Eigen::Vector3d& v,
    WorldFrame csvWorld)
{
    return ConvertWorldVector(
        v,
        csvWorld,
        CANONICAL_WORLD_FRAME);
}
