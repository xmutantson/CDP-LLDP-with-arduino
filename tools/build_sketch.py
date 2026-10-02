"""Stage the Arduino-required sketch folder and compile only (never upload)."""
import argparse
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cli", default="arduino-cli")
args = parser.parse_args()
staged = ROOT / ".build" / "LCD_Listener_Nano"
staged.mkdir(parents=True, exist_ok=True)
for name in ("LCD_Listener_Nano.ino", "DiscoveryParser.h"):
    shutil.copyfile(ROOT / name, staged / name)
subprocess.run([
    args.cli, "compile", "--fqbn", "arduino:avr:nano:cpu=atmega328old",
    "--warnings", "all", "--build-path", str(ROOT / ".build" / "avr"), str(staged),
], check=True)
