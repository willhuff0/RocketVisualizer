"""Exercise the same stdin producer the render loop uses, without a window."""
import json
import subprocess
import sys
import time

receiver = sys.argv[1]

# An open, idle pipe must never block the render loop or window shutdown.
idle = subprocess.Popen([receiver, "--poll-once"], stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
try:
    assert idle.wait(timeout=2) == 0
    result = json.loads(idle.stdout.read())
    assert result == {"count": 0, "finished": False}, result
finally:
    idle.stdin.close()
    idle.stdin = None
    if idle.poll() is None:
        idle.kill()
    idle.communicate()

process = subprocess.Popen([receiver, "--stdin"], stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
try:
    # Split a valid record across writes, intermixed with firmware diagnostics.
    process.stdin.write(b"Flight controller ready.\nAHRS1,10,1,2,-3,1,0")
    process.stdin.flush()
    time.sleep(0.02)
    process.stdin.write(b",0,0\r\nAHRS1,bad\n" + b"X" * 600 + b"\n")
    # A restarted source clears history; a repeated timestamp replaces a sample.
    process.stdin.write(b"AHRS1,0,4,5,-6,1,0,0,0\n"
                        b"AHRS1,1,7,8,-9,1,0,0,0\n"
                        b"AHRS1,1,10,11,-12,1,0,0,0\n"
                        b"AHRS1,2,100,100,-100,1,0")  # incomplete at EOF
    process.stdin.close()
    process.stdin = None
    output, error = process.communicate(timeout=3)
    assert process.returncode == 0, error.decode()
    result = json.loads(output)
    assert result["count"] == 4 and result["finished"], result
    assert result["begin"] == 0 and result["end"] == 1, result
    assert result["position"] == [10, -11, 12], result
    assert result["nose"] == [1, 0, 0], result
finally:
    if process.poll() is None:
        process.kill()
        process.communicate()

empty = subprocess.run([receiver, "--stdin"], input=b"", capture_output=True, timeout=2, check=True)
assert json.loads(empty.stdout) == {"count": 0, "finished": True}
print("AHRS stdin tests passed")
