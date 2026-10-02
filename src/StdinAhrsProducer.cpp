#include "StdinAhrsProducer.hpp"
#include "AhrsTelemetryParser.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#endif

#include <iostream>

StdinAhrsProducer::StdinAhrsProducer(std::shared_ptr<LiveRocketTimeline> timeline)
    : _timeline(std::move(timeline)) {
    if (!_timeline) throw std::invalid_argument("AHRS producer requires a live timeline");
}

void StdinAhrsProducer::Consume(const char* data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) {
        const char c = data[i];
        if (c == '\n') {
            if (!_discardLine) {
                const auto sample = ParseAhrsTelemetry(_line);
                if (sample) {
                    // USB reconnect/controller restart begins a fresh source clock.
                    if (_hasSample && sample->time < _lastTime) _timeline->Clear();
                    _timeline->Publish(*sample);
                    _hasSample = true;
                    _lastTime = sample->time;
                    ++_sampleCount;
                }
            }
            _line.clear();
            _discardLine = false;
        } else if (!_discardLine) {
            if (_line.size() >= 512) {
                _line.clear();
                _discardLine = true;
            } else {
                _line += c;
            }
        }
    }
}

void StdinAhrsProducer::Poll() {
    if (_finished) return;
    // Limit work per frame even if the producer floods the pipe.
    for (int chunk = 0; chunk < 8; ++chunk) {
        char buffer[4096];
#ifdef _WIN32
        const HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
        const DWORD type = GetFileType(input);
        DWORD available = sizeof(buffer);
        if (type == FILE_TYPE_PIPE) {
            if (!PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr)) {
                _finished = true;
                if (GetLastError() != ERROR_BROKEN_PIPE) _error = "Could not inspect stdin pipe";
                return;
            }
            if (available == 0) return;
        } else if (type != FILE_TYPE_DISK) {
            _error = "--live-stdin requires a pipe or redirected file on Windows";
            _finished = true;
            return;
        }
        DWORD count = 0;
        if (!ReadFile(input, buffer, (std::min)(available, static_cast<DWORD>(sizeof(buffer))), &count, nullptr)) {
            _finished = true;
            if (GetLastError() != ERROR_BROKEN_PIPE) _error = "Could not read stdin";
            return;
        }
#else
        pollfd input{STDIN_FILENO, POLLIN, 0};
        const int ready = poll(&input, 1, 0);
        if (ready < 0) {
            if (errno == EINTR) return;
            _error = std::strerror(errno);
            _finished = true;
            return;
        }
        if (ready == 0) return;
        const auto count = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (count < 0) {
            if (errno == EINTR || errno == EAGAIN) return;
            _error = std::strerror(errno);
            _finished = true;
            return;
        }
#endif
        if (count == 0) {
            _finished = true;
            _line.clear(); // drop incomplete records
            return;
        }
        Consume(buffer, static_cast<std::size_t>(count));
    }
}
