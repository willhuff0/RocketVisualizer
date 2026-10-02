#!/usr/bin/env python3
"""Forward a serial port to RocketVisualizer or stdout without a terminal monitor."""
import argparse
import os
import subprocess
import sys


def forward(port, output, finished):
    while not finished():
        # Wait for the first byte, then drain available data without waiting to
        # fill a buffer. This keeps 50 Hz telemetry responsive even on an idle port.
        data = port.read(1)
        if data:
            data += port.read(min(port.in_waiting, 4095))
            output.write(data)
            output.flush()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Serial port, e.g. /dev/cu.usbmodem123456 or COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--list-ports", action="store_true", help="List ports and exit")
    parser.add_argument("--visualizer", help="Path to RocketVisualizer; otherwise stream raw bytes to stdout")
    args = parser.parse_args()
    if args.baud <= 0:
        parser.error("--baud must be positive")
    if not args.list_ports and not args.port:
        parser.error("--port is required unless --list-ports is used")

    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print("serial_telemetry: pyserial is required; install tools/requirements.txt with this Python interpreter",
              file=sys.stderr)
        return 1

    if args.list_ports:
        for port in list_ports.comports():
            print(f"{port.device}\t{port.description}")
        return 0

    child = None
    try:
        with serial.Serial(args.port, args.baud, timeout=0.1) as port:
            output = sys.stdout.buffer
            if args.visualizer:
                # An independent process group lets Ctrl+C stop the reader while
                # preserving the visualizer's buffered history after EOF.
                options = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
                           if os.name == "nt" else {"start_new_session": True})
                child = subprocess.Popen([args.visualizer, "--live-stdin"], stdin=subprocess.PIPE, **options)
                output = child.stdin
            print(f"Reading {args.port} at {args.baud} baud. Ctrl+C stops the reader.", file=sys.stderr)
            forward(port, output, lambda: child is not None and child.poll() is not None)
        return 0 if child is None or child.returncode == 0 else 1
    except KeyboardInterrupt:
        return 0
    except BrokenPipeError:
        if child is None:
            # Avoid a second BrokenPipeError when Python flushes stdout on exit.
            with open(os.devnull, "wb") as sink:
                os.dup2(sink.fileno(), sys.stdout.fileno())
        return 0 if child is None or child.poll() in (None, 0) else 1
    except (serial.SerialException, OSError) as error:
        print(f"serial_telemetry: {error}", file=sys.stderr)
        return 1
    finally:
        if child is not None and child.stdin is not None:
            try:
                child.stdin.close()
            except BrokenPipeError:
                pass


if __name__ == "__main__":
    sys.exit(main())
