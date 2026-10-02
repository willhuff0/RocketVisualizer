"""Verify the direct reader with virtual serial ports; no Teensy or GUI required."""
import json
import os
from pathlib import Path
import select
import shlex
import signal
import subprocess
import sys
import tempfile
import time

try:
    import serial
except ImportError:
    print("Direct serial tests require pyserial (tools/requirements.txt)")
    sys.exit(77)
if os.name == "nt":
    print("Virtual serial port tests require POSIX pseudo-terminals")
    sys.exit(77)
import pty

script, receiver = sys.argv[1:3]


def start(*extra):
    master, slave = pty.openpty()
    try:
        process = subprocess.Popen([sys.executable, script, "--port", os.ttyname(slave), *extra],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ready, _, _ = select.select([process.stderr], [], [], 5)
        assert ready, "Reader did not open the port"
        status = process.stderr.readline()
        assert status.startswith(b"Reading "), status.decode()
        return process, master, slave
    except BaseException:
        if "process" in locals():
            process.kill()
            process.communicate()
        os.close(master)
        os.close(slave)
        raise


def finish(process, master, slave):
    if process.poll() is None:
        process.kill()
    process.communicate()
    if master is not None:
        os.close(master)
    os.close(slave)


# Read through an actual pseudo-terminal into the real C++ timeline producer.
process, master, slave = start()
timeline = None
try:
    timeline = subprocess.Popen([receiver, "--stdin"], stdin=process.stdout,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    process.stdout.close()
    process.stdout = None
    os.write(master, b"Flight controller ready.\nAHRS1,0,1,2,-3,1,0")
    time.sleep(0.02)
    os.write(master, b",0,0\nAHRS1,1,4,5,-6,1,0,0,0\n")
    time.sleep(0.1)
    process.send_signal(signal.SIGINT)
    _, error = process.communicate(timeout=3)
    assert process.returncode == 0 and not error, error.decode()
    output, error = timeline.communicate(timeout=3)
    assert timeline.returncode == 0, error.decode()
    result = json.loads(output)
    assert result["count"] == 2 and result["finished"], result
    assert result["position"] == [4, -5, 6] and result["end"] == 1, result
finally:
    if timeline is not None:
        if timeline.poll() is None:
            timeline.kill()
        timeline.communicate()
    finish(process, master, slave)

# Managed mode must stop and release an idle port after the viewer exits.
with tempfile.TemporaryDirectory() as directory:
    child = Path(directory) / "viewer.py"
    child.write_text("import sys\nassert sys.argv[1:] == ['--live-stdin']\n"
                     "for line in sys.stdin.buffer:\n"
                     "    if line.startswith(b'AHRS1,'):\n"
                     "        print('Viewer received telemetry', flush=True)\n"
                     "        break\n")
    launcher = Path(directory) / "viewer"
    launcher.write_text(f"#!/bin/sh\nexec {shlex.quote(sys.executable)} {shlex.quote(str(child))} \"$@\"\n")
    launcher.chmod(0o755)
    process, master, slave = start("--visualizer", str(launcher))
    try:
        os.write(master, b"AHRS1,0,0,0,0,1,0,0,0\n")
        output, error = process.communicate(timeout=3)
        assert process.returncode == 0 and output == b"Viewer received telemetry\n", (output, error)
        assert not error, error.decode()
    finally:
        finish(process, master, slave)

    # A viewer that fails immediately must produce a failure status even if
    # the serial port is idle.
    child.write_text("import sys\nsys.exit(4)\n")
    process, master, slave = start("--visualizer", str(launcher))
    try:
        _, error = process.communicate(timeout=3)
        assert process.returncode == 1 and not error, error.decode()
    finally:
        finish(process, master, slave)

# A closed stdout consumer must exit quietly instead of printing a traceback.
process, master, slave = start()
process.stdout.close()
process.stdout = None
try:
    os.write(master, b"AHRS1,0,0,0,0,1,0,0,0\n")
    _, error = process.communicate(timeout=3)
    assert process.returncode == 0 and not error, error.decode()
finally:
    finish(process, master, slave)

# Unplugging the device must fail explicitly and close the reader.
process, master, slave = start()
try:
    os.close(master)
    master = None
    _, error = process.communicate(timeout=3)
    assert process.returncode == 1 and b"serial_telemetry:" in error, error.decode()
    assert b"Traceback" not in error, error.decode()
finally:
    finish(process, master, slave)

bad = subprocess.run([sys.executable, script, "--port", "/no-such-serial-device"],
                     capture_output=True, timeout=3)
assert bad.returncode == 1 and b"serial_telemetry:" in bad.stderr, bad.stderr.decode()
assert not bad.stdout
listed = subprocess.run([sys.executable, script, "--list-ports"],
                        capture_output=True, timeout=3)
assert listed.returncode == 0 and not listed.stderr, listed.stderr.decode()
print("Direct serial integration tests passed")
