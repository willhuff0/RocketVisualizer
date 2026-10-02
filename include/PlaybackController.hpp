#pragma once

#include "IRocketTimeline.hpp"

enum class PlaybackMode {
    FollowLive,
    Playing,
    Paused,
};

class PlaybackController {
public:
    void Update(double dt, TimeRange availableRange);

    void Seek(double time);
    void SetSpeed(double speed);

    void FollowLive();
    void Play();
    void Pause();

    PlaybackMode GetPlaybackMode() const;
    double GetCursor() const;
    double GetSpeed() const;

private:
    PlaybackMode _mode = PlaybackMode::FollowLive;
    double _cursor = 0.0;
    double _speed = 1.0;
};