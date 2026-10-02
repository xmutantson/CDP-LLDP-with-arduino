# CDP-LLDP-with-arduino
This repository is for the CDP project originally started here: http://www.modlog.net/?p=907 and then revisited and LLDP added here: http://www.modlog.net/?p=1591

The original code was by Chris van Marle - CDPSniffino: https://github.com/qistoph/CdpSniffino. It was butchered to suit the ENC28J60 chipset with a red PCB ST7735 LCD.

Xmutantson adapted the original display to a 1602A LCD. This version retains that
seven-window layout and adds bounded, fixed-memory parsing and LLDP LCD output.

## Display and parsing

The first two windows show local DHCP/gateway information (or startup errors).
The next five show the most recently accepted advertisement's device/system name,
source MAC, port, model and VLAN. Both CDP and LLDP update these windows. Every
accepted message replaces all neighbor fields, including empty fields, so values
from different neighbors/protocols are not mixed. Incomplete receives leave the
previous snapshot intact. Management IPv4 and voice VLAN are printed on serial at
9600 baud, retaining the seven-window LCD cycle.

`DiscoveryParser.h` uses seven fixed 17-byte strings with explicit terminators,
without Arduino `String` or heap allocation. Device names show their first 16
characters; port/model names retain their last 16, preserving the previous intent.
Non-printable/non-ASCII bytes become `?`. MAC addresses use 12 hex digits to fit.
Missing optional fields stay blank; a system description is not guessed to be a model.

Supported input is untagged Ethernet CDP (802.3 LLC/SNAP) and LLDP (EtherType
0x88cc). CDP lengths are bounded by the advertised 802.3 payload and actual receive
length. LLDP uses its full nine-bit TLV length and requires the three mandatory TLVs
in order plus an end marker. Unknown bounded TLVs are skipped. Address records and
organizational TLVs are checked before access. The parser does not authenticate
advertisements or verify the CDP checksum; the ENC28J60 driver checks receive status.

The 500-byte receive buffer admits at most 499 bytes through EtherCard 1.1.0.
A message whose required content is truncated is ignored. Ethernet padding after
the CDP payload or LLDP end marker is ignored. VLAN-tagged frames and IPv6 management
display are outside this version's scope. The first supported IPv4 address is shown.
LLDP-MED voice VLAN is displayed only for a defined tagged voice policy.

DHCP is retained: **running the sketch on hardware sends DHCP traffic during startup**.
Discovery reception is enabled even if DHCP fails. An ENC28J60 initialization failure
instead displays a wiring error and stops receive calls.

## Wiring for the existing Nano / 1602A adaptation

These assignments come from the sketch, not a validated hardware schematic.

| Peripheral signal | Arduino Nano pin |
| --- | --- |
| ENC28J60 CS | D10 (`ether.begin(..., 10)`) |
| ENC28J60 MOSI / SI | D11 |
| ENC28J60 MISO / SO | D12 |
| ENC28J60 SCK | D13 |
| LCD RS | A5 |
| LCD Enable | A4 |
| LCD D4 / D5 / D6 / D7 | A0 / A1 / A2 / A3 |

The LCD uses parallel four-bit mode, not I2C. Its R/W is normally tied to ground;
V0 contrast, power, ground and backlight connections depend on the actual display.
Use the module specifications for supply voltage, logic compatibility and backlight
current limiting. Bare ENC28J60 power is 3.3 V; a breakout's regulator/level shifting
must be checked rather than inferred from its connector. The old unused `cs = 3`
define referred to neither the active ENC28J60 initialization nor this LCD and has
been removed. See [EtherCard's wiring reference](https://github.com/njh/EtherCard#physical-installation).

The existing `mymac` remains unchanged; configure a unique address before hardware
use. No board, wiring, power, LCD timing or physical reception has been validated
by the offline checks below.

## Offline tests

Install Python 3 and a C++11-capable GNU-compatible compiler, then run:

```text
python tests/run_tests.py
```

`CXX` can select a compiler executable. The runner builds in a temporary directory.
It tests normal synthetic advertisements and incomplete receive buffers in memory,
plus the actual sketch using mocked EtherCard, LiquidCrystal and Arduino calls.
It does not use a network interface, serial device or real packet capture.

On Windows the test runtime is statically linked to avoid unrelated MinGW DLLs on
PATH. Failed checks print an error and exit normally; the runner suppresses Windows
error dialogs for its own child processes only. These are host test precautions,
not changes to the board firmware or system settings.

## Reproducible compile-only build

Validated toolchain: Arduino CLI **1.3.1**, Arduino AVR Boards **1.8.6**,
EtherCard **1.1.0**, LiquidCrystal **1.0.7**. Target:
`arduino:avr:nano:cpu=atmega328old`. No upload command is provided here.

Use an isolated Arduino data/user directory so this does not change an existing IDE
installation. For example, in PowerShell from the repository:

```powershell
$buildTools = Join-Path $PWD '.build/arduino-tools'
$env:ARDUINO_DIRECTORIES_DATA = Join-Path $buildTools 'data'
$env:ARDUINO_DIRECTORIES_DOWNLOADS = Join-Path $buildTools 'downloads'
$env:ARDUINO_DIRECTORIES_USER = Join-Path $buildTools 'user'
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install EtherCard@1.1.0 LiquidCrystal@1.0.7
python tools/build_sketch.py
```

Dependency installation requires downloads from Arduino's registry. With those
dependencies present, `tools/build_sketch.py` stages the correctly named sketch
folder and runs compilation only. `--cli /path/to/arduino-cli` selects a CLI binary.
It never calls upload, board discovery or a serial monitor. Build outputs stay in
`.build/` and are ignored by Git.

The checked build uses **12,194 bytes flash (39%)** and **1,263 bytes static SRAM
(61%)**, leaving **785 bytes** for stack and runtime use on the 2 KB Nano. Display
labels reside in flash. This is compiler accounting, not a measurement of peak
stack usage; physical soak testing remains necessary.

## Remaining behavior decisions

This remains a last-advertisement viewer, not a neighbor table. It retains the last
accepted snapshot until another arrives: TTL aging, shutdown advertisements,
link-loss clearing, neighbor selection and more LCD windows need explicit product
decisions. No timers or invented timeout policy have been added. See
[validation notes](docs/VALIDATION.md) for the tested scope and limitations.
