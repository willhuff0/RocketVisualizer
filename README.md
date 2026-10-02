# RocketVisualizer

The visualizer can import pose CSVs through its File menu, display the default
demo, or follow the controller's AHRS output live. The live path is a text pipe:
controller stdout or Teensy USB serial feeds `--live-stdin`, which publishes
samples into the existing `LiveRocketTimeline`. No extra server is required.

## Build

Requires CMake 3.25+, a C++20 compiler, and Python 3 for tests. CMake downloads
pinned Eigen, raylib, ImGui and rlImGui dependencies if needed.
From the repository root:

```sh
cmake -S RocketVisualizer -B RocketVisualizer/build
cmake --build RocketVisualizer/build
ctest --test-dir RocketVisualizer/build --output-on-failure
```

For timeline and ingestion tests without a graphics dependency, configure with
`-DROCKET_VISUALIZER_BUILD_APP=OFF`. Resources are located from the source tree,
so the executable can be launched from the repository root or another directory.
For multi-configuration generators, build with `--config Debug` and use the
executable under `build/Debug` in the commands below.

## Connect a controller

Build the desktop controller using [its README](../FlightController/README.md).
For CSV replay at the original sample pace:

```sh
FlightController/build/desktop/FlightController --csv FlightController/examples/sensors.csv --realtime --telemetry | RocketVisualizer/build/RocketVisualizer --live-stdin
```

For the OpenRocket bridge, start this pipeline, then run the simulation with the
TCP Bridge extension enabled:

```sh
FlightController/build/desktop/FlightController --openrocket --telemetry | RocketVisualizer/build/RocketVisualizer --live-stdin
```

For a Teensy with the new firmware, use the direct serial reader. PlatformIO's
device monitor requires a live terminal and cannot supply this pipe. Set up the
reader once (on Windows, use `RocketVisualizer\.venv\Scripts\python.exe` for the
venv interpreter):

```sh
python3 -m venv RocketVisualizer/.venv
RocketVisualizer/.venv/bin/python -m pip install -r RocketVisualizer/tools/requirements.txt
RocketVisualizer/.venv/bin/python RocketVisualizer/tools/serial_telemetry.py --list-ports
```

Then substitute the actual port and launch the visualizer with the reader:

```sh
RocketVisualizer/.venv/bin/python RocketVisualizer/tools/serial_telemetry.py --port /dev/cu.usbmodem123456 --visualizer RocketVisualizer/build/RocketVisualizer
```

On Windows the port will look like `COM3`, and the visualizer path ends in `.exe`.
Close other monitors using the port before connecting. The reader uses 115200
baud by default; override it with `--baud`. It reads the serial port directly,
forwards bytes immediately, and closes the port when the visualizer exits.
Ctrl+C stops the reader; the visualizer stays open with the received history.
Disconnects and open failures are reported on stderr. To use a shell pipe or
record the raw stream instead, omit `--visualizer` to emit serial bytes on stdout:

```sh
RocketVisualizer/.venv/bin/python RocketVisualizer/tools/serial_telemetry.py --port /dev/cu.usbmodem123456 | RocketVisualizer/build/RocketVisualizer --live-stdin
```

The `ahrs_serial` CTest checks the reader using virtual serial ports on macOS/Linux
when the configured Python has pyserial installed; otherwise it is skipped. To
run that test explicitly with the reader's venv:

```sh
RocketVisualizer/.venv/bin/python RocketVisualizer/tests/serial_integration.py RocketVisualizer/tools/serial_telemetry.py RocketVisualizer/build/ahrs_timeline_tests
```

For shell pipelines on Windows use `cmd.exe` to preserve streaming text.
The visualizer follows the newest sample by default. Pause or scrub to inspect
buffered history; **Timeline → Go To Live** or **End** resumes following. When
input ends, the window stays open with the buffered timeline. A decreasing source
timestamp clears history for a controller restart.

## Telemetry format

Each record ends in a newline:

```text
AHRS1,time_seconds,north_m,east_m,down_m,qw,qx,qy,qz
```

The quaternion maps controller body FRD (+X nose, +Y right, +Z down) into world
NED (+X north, +Y east, +Z down). The receiver converts both position and attitude
to the visualizer's NWU world and RDF body frames; no import-dialog settings are
needed. The timestamp is the sensor or simulation clock, not arrival time.

The controller sends its current estimated attitude and position, with barometric
altitude replacing estimated vertical position when available. Horizontal position
comes from the controller's existing inertial estimate and can drift; there is
no GPS position correction. Unavailable position is sent as zero, and records
without a valid attitude are skipped. This stream does not contain fin commands.

Serial diagnostics and malformed records are ignored. The reader handles records
split across reads, discards oversized or incomplete lines, and polls available
input from the render loop so an idle pipe does not freeze playback or shutdown.
