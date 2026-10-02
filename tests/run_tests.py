"""Compile/run synthetic parser and mocked sketch tests; never access a device/network."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

if os.name == "nt":
    import ctypes
    # Process-local and inherited by children; preserve diagnostics/exit status,
    # but never display a Windows Error Reporting dialog during unattended tests.
    ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)

with tempfile.TemporaryDirectory(prefix="discovery-tests-") as temporary:
    for name in ("parser_test", "sketch_test"):
        output = Path(temporary) / (name + (".exe" if os.name == "nt" else ""))
        # Avoid accidentally loading an unrelated MinGW runtime from a Python PATH.
        runtime_flags = ["-static-libgcc", "-static-libstdc++"] if os.name == "nt" else []
        subprocess.run([
            os.environ.get("CXX", "g++"), "-std=c++11", "-Wall", "-Wextra",
            "-Werror", "-pedantic", "-O2", "-I", str(ROOT / "tests" / "stubs"),
            str(ROOT / "tests" / (name + ".cpp")), "-o", str(output),
        ] + runtime_flags, check=True)
        subprocess.run([str(output)], check=True)
