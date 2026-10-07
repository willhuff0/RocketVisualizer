#include <CsvTimelineLoader.hpp>

#include "CoordinateConverter.hpp"

#include <portable-file-dialogs.h>
#include <rapidcsv.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <filesystem>
#include <vector>
#include <unordered_set>
#include <algorithm>

constexpr float NAME_FIELD_WIDTH = 80.0f;

const CsvImportConfig OPEN_ROCKET_PROFILE = {
    .configName = "OpenRocket (TcpBridge)",
    .coordinateSystem = {
        .worldFrame = WorldFrame::ENU,
        .bodyFrame = BodyFrame::OpenRocket,
        .rotationDirection = RotationDirection::BodyToWorld,
    },
    .timeName = "##0",
    .stateMachineStateName = "",
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

const CsvImportConfig FLIGHT_CSV_PROFILE = {
    .configName = "Sounding Rocket 10/5/26",
    .coordinateSystem = {
        .worldFrame = WorldFrame::NED,
        .bodyFrame = BodyFrame::FLU,
        .rotationDirection = RotationDirection::BodyToWorld,
    },
    .timeName = "time_s",
    .stateMachineStateName = "state",
    .quaternion = {
        .wName = "q_body2ned_w",
        .xName = "q_body2ned_x",
        .yName = "q_body2ned_y",
        .zName = "q_body2ned_z",
    },
    .position = {
        .xName = "pos_n_m",
        .yName = "pos_e_m",
        .zName = "pos_d_m",
    },
};

const CsvImportConfig OLD_FLIGHT_CSV_PROFILE = {
    .configName = "VDF2_03072026_FLIGHT_IDX",
    .coordinateSystem = {
        .worldFrame = WorldFrame::SEU,
        .bodyFrame = BodyFrame::BRU,
        .rotationDirection = RotationDirection::BodyToWorld,
    },
    .timeName = "time_s",
    .stateMachineStateName = "",
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
    FLIGHT_CSV_PROFILE,
    OLD_FLIGHT_CSV_PROFILE
};

static std::string selectCsvFile() {
    auto result = pfd::open_file("Select a CSV.", "", {
        "*.csv",
    }).result();
    if (result.empty()) return "";
    return result.front();
}

template<typename T>
static std::vector<T> getColumn(
    const std::string& name,
    const rapidcsv::Document& doc,
    const std::unordered_set<std::string>& columnNames)
{
    if (name.starts_with("##")) {
        auto columnIndexStr = name.substr(2);
        if (!columnIndexStr.empty()) {
            try {
                int columnIndex = std::stoi(columnIndexStr);
                return doc.GetColumn<T>(static_cast<size_t>(columnIndex));
            }
            catch (const std::exception& _) {}
        }
    }

    return columnNames.contains(name) ? doc.GetColumn<T>(name) : std::vector<T>();
}

static std::shared_ptr<VectorRocketTimeline> loadTimelineFromCsv(const std::string& path, CsvImportConfig config) {
    try {
        rapidcsv::Document doc(path);

        auto columnNamesVec = doc.GetColumnNames();
        auto columnNames = std::unordered_set(columnNamesVec.begin(), columnNamesVec.end());

        auto t = getColumn<double>(config.timeName, doc, columnNames);

        auto qw = getColumn<double>(config.quaternion.wName, doc, columnNames);
        auto qx = getColumn<double>(config.quaternion.xName, doc, columnNames);
        auto qy = getColumn<double>(config.quaternion.yName, doc, columnNames);
        auto qz = getColumn<double>(config.quaternion.zName, doc, columnNames);
        size_t qMinLength = (std::min)({qw.size(), qx.size(), qy.size(), qz.size()});

        auto px = getColumn<double>(config.position.xName, doc, columnNames);
        auto py = getColumn<double>(config.position.yName, doc, columnNames);
        auto pz = getColumn<double>(config.position.zName, doc, columnNames);
        size_t pMaxLength = (std::max)({px.size(), py.size(), pz.size()});

        std::vector<double> actualFinAngles[MAX_FIN_COUNT];
        std::vector<double> targetFinAngles[MAX_FIN_COUNT];
        size_t finAnglesMaxLength = 0;
        for (size_t j = 0; j < MAX_FIN_COUNT; j++) {
            actualFinAngles[j] = getColumn<double>(config.fins[j].actualAngleName, doc, columnNames);
            targetFinAngles[j] = getColumn<double>(config.fins[j].targetAngleName, doc, columnNames);

            finAnglesMaxLength = (std::max)({finAnglesMaxLength, actualFinAngles[j].size(), targetFinAngles[j].size()});
        }

        auto stateMachineStates = getColumn<std::string>(config.stateMachineStateName, doc, columnNames);

        size_t sampleCount = (std::min)(t.size(), (std::max)({qMinLength, pMaxLength, finAnglesMaxLength}));
        std::vector<TimedRocketState> samples;
        samples.reserve(sampleCount);
        for (size_t i = 0; i < sampleCount; i++) {
            TimedRocketState sample;

            sample.time = t[i];

            if (qMinLength != 0) {
                size_t qIndex = (std::min)(i, qMinLength - 1);
                sample.state.orientation = Eigen::Quaterniond {
                    qw[qIndex],
                    qx[qIndex],
                    qy[qIndex],
                    qz[qIndex],
                };
                sample.state.orientation = CoordinateConverter::ImportQuaternion(
                    sample.state.orientation,
                    config.coordinateSystem.worldFrame,
                    config.coordinateSystem.bodyFrame,
                    config.coordinateSystem.rotationDirection
                );
            }

            if (!px.empty()) {
                size_t pxIndex = (std::min)(i, px.size() - 1);
                sample.state.position.x() = px[pxIndex];
            }
            if (!py.empty()) {
                size_t pyIndex = (std::min)(i, py.size() - 1);
                sample.state.position.y() = py[pyIndex];
            }
            if (!pz.empty()) {
                size_t pzIndex = (std::min)(i, pz.size() - 1);
                sample.state.position.z() = pz[pzIndex];
            }
            sample.state.position = CoordinateConverter::ImportWorldVector(
                sample.state.position,
                config.coordinateSystem.worldFrame
            );

            for (size_t j = 0; j < MAX_FIN_COUNT; j++) {
                auto& actualFinAngleVector = actualFinAngles[j];
                if (!actualFinAngleVector.empty()) {
                    size_t actualFinAngleIndex = (std::min)(i, actualFinAngleVector.size() - 1);
                    sample.state.actualFinAngles[j] = actualFinAngleVector[actualFinAngleIndex];
                }

                auto& targetFinAngleVector = targetFinAngles[j];
                if (!targetFinAngleVector.empty()) {
                    size_t targetFinAngleIndex = (std::min)(i, targetFinAngleVector.size() - 1);
                    sample.state.targetFinAngles[j] = targetFinAngleVector[targetFinAngleIndex];
                }
            }

            sample.state.stateMachineState = stateMachineStates[i];

            samples.emplace_back(sample);
        }

        return std::make_shared<VectorRocketTimeline>(std::move(samples));
    } catch (const std::exception& ex) {
        std::cerr << "Failed to open or parse CSV: " << ex.what() << std::endl;
        return nullptr;
    }
}

CsvTimelineLoader::CsvTimelineLoader(std::function<void(RocketConfig, std::shared_ptr<VectorRocketTimeline>)> onImport)
    : _onImport(std::move(onImport)) { }

void CsvTimelineLoader::ShowImportDialog() {
    if (_path.empty()) _path = selectCsvFile();
    if (_path.empty()) return;
    ImGui::OpenPopup(_popupId);
}

void CsvTimelineLoader::DrawUI() {
    _popupId = ImGui::GetID("Import CSV");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    bool isModalOpen = true;
    if (ImGui::BeginPopupModal("Import CSV", &isModalOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {
        if (ImGui::Button("Select File")) {
            std::string newPath = selectCsvFile();
            if (!newPath.empty()) {
                _path = newPath;
            }
        }
        ImGui::SameLine();
        auto fileName = std::filesystem::path(_path).filename().string();
        ImGui::TextUnformatted(fileName.c_str());
        ImGui::Separator();

        if (ImGui::BeginCombo("##PresetsCombo", "Presets")) {
            for (const auto& profile : CSV_IMPORT_PROFILES) {
                if (ImGui::Selectable(profile.configName.c_str())) {
                    _config = profile;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        CoordinateSystemImportUI::Draw(_config.coordinateSystem);

        ImGui::Separator();
        ImGui::Text("Time");
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("t##Time", &_config.timeName);

        ImGui::Separator();
        ImGui::Text("State");
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("state##StateMachineState", &_config.stateMachineStateName);

        ImGui::Separator();
        ImGui::Text("Orientation");

        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("w##Quaternion", &_config.quaternion.wName);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("x##Quaternion", &_config.quaternion.xName);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("y##Quaternion", &_config.quaternion.yName);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("z##Quaternion", &_config.quaternion.zName);

        ImGui::Separator();
        ImGui::Text("Position");

        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("x##Position", &_config.position.xName);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("y##Position", &_config.position.yName);

        ImGui::SameLine();
        ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
        ImGui::InputText("z##Position", &_config.position.zName);

        ImGui::Separator();
        _drawFinList();

        ImGui::Separator();
        ImGui::BeginDisabled(_path.empty());
        if (ImGui::Button("Import")) {
            auto result = loadTimelineFromCsv(_path, _config);
            if (result != nullptr) {
                RocketConfig rocketConfig;
                for (size_t j = 0; j < MAX_FIN_COUNT; j++) {
                    rocketConfig.finRotationAxes[j] = _config.fins[j].rotationAxis;
                    rocketConfig.finNodeNames[j] = _config.fins[j].nodeName;
                }

                _onImport(rocketConfig, result);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }
}

void CsvTimelineLoader::_drawFinList() {
    if (ImGui::CollapsingHeader("Fins")) {
        ImGui::TextUnformatted("   Model    | Column Names");

        float buttonStart = ImGui::GetWindowWidth() - 50.0f;
        ImGui::SameLine(buttonStart);
        if (ImGui::Button(" + ")) {
            _config.finCount = (std::min)(_config.finCount + 1, MAX_FIN_COUNT);
        }

        ImGui::TextUnformatted("   Node     | Actual Angle, Target Angle");

        if (ImGui::BeginChild("##FinListRegion", ImVec2(0.0f, 124.0f))) {
            for (size_t i = 0; i < _config.finCount; i++) {
                ImGui::PushID(i);

                ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
                ImGui::InputText("##finNodeName", &_config.fins[i].nodeName);

                ImGui::SameLine();
                ImGui::Text(" : ");

                ImGui::SameLine();
                ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
                ImGui::InputText("##finActualAngleName", &_config.fins[i].actualAngleName);

                ImGui::SameLine();
                ImGui::Text(",");

                ImGui::SameLine();
                ImGui::SetNextItemWidth(NAME_FIELD_WIDTH);
                ImGui::InputText("##finTargetAngleName", &_config.fins[i].targetAngleName);

                ImGui::SameLine(buttonStart - 8.0f);
                if (ImGui::Button(" X ##deleteFin")) {
                    std::move(_config.fins + i + 1, _config.fins + MAX_FIN_COUNT, _config.fins + i);
                    _config.finCount = (std::max<size_t>)(_config.finCount - 1, 0);
                    _config.fins[_config.finCount].~CsvFinConfig();
                    _config.fins[_config.finCount] = {};
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();
    }
}
