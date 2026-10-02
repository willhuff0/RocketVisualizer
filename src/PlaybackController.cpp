#include <PlaybackController.hpp>

#include <algorithm>

void PlaybackController::Update(double dt, TimeRange availableRange) {
    switch (_mode) {
        case PlaybackMode::FollowLive:
            _cursor = availableRange.end;
            break;
        case PlaybackMode::Playing:
            _cursor = std::clamp(_cursor + dt * _speed, availableRange.begin, availableRange.end);
            break;
        case PlaybackMode::Paused:
            break;
    }
}

void PlaybackController::Seek(double time) {
    _cursor = time;
}

void PlaybackController::SetSpeed(double speed) {
    _speed = speed;
}

void PlaybackController::FollowLive() {
    _mode = PlaybackMode::FollowLive;
}

void PlaybackController::Play() {
    _mode = PlaybackMode::Playing;
}

void PlaybackController::Pause() {
    _mode = PlaybackMode::Paused;
}

PlaybackMode PlaybackController::GetPlaybackMode() const {
    return _mode;
}

double PlaybackController::GetCursor() const {
    return _cursor;
}

double PlaybackController::GetSpeed() const {
    return _speed;
}
