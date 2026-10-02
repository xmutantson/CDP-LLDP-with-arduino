# Offline validation, 2026-10-02

Base: `c48aff40eb7587178ec981f91db8e54df95c3351`, the current fork master fetched
into an independent task-local checkout. No applicable `AGENTS.md` was present.

## Results

- `python tests/run_tests.py`: **13 passing test groups** in two executables,
  compiled using g++ 13.2.0, C++11, `-Wall -Wextra -Werror -pedantic -O2`.
- `python tools/build_sketch.py --cli <isolated Arduino CLI 1.3.1>`: **PASS** for
  Nano ATmega328P old bootloader, AVR core 1.8.6, EtherCard 1.1.0, LiquidCrystal 1.0.7.
  Flash 12,194 / 30,720 bytes; static SRAM 1,263 / 2,048 bytes, 785 bytes remaining.
- `git diff --check`: **PASS** (Git may report a Windows line-ending conversion notice).
- No added GitHub workflows or paid CI. No upload, hardware I/O, packet transmission,
  IR/radio use, network discovery or live service tests.

The parser checks normal CDP and LLDP fields, multi-byte voice VLAN values, a valid
LLDP description longer than 255 bytes, complete snapshot replacement, fixed-width
and terminated strings, printable LCD output, incomplete receive handling, Ethernet
padding, LLDP destination scopes, unrelated Ethernet traffic and mixed address
families. Mocked sketch tests cover controller failure, DHCP failure/success, CDP
and LLDP LCD updates, stale character clearing and preserving the display after
an incomplete receive. Fixtures are synthetic and remain in process memory.

Arduino's first full build emitted unused-parameter/unused-label warnings in
EtherCard and Arduino AVR core sources. There were no warnings attributed to this
sketch/parser. The final cached build completed without new diagnostics. Compiler
SRAM accounting is not a stack high-water measurement or hardware validation.

## Host test failure and correction

The first Windows parser test **failed**, with a visible application-error dialog.
GDB reproduced SIGSEGV at `0x000000006FCAE829` in radioconda's
`Library/mingw-w64/bin/libstdc++-6.dll`, called from the first `std::cout` in the test
after its CDP checks. The test was built with Strawberry MinGW g++ 13.2.0 but loaded
an incompatible C++ runtime from PATH. The diagnostic binary was disabled and was
not counted as a passing run.

Windows tests now statically link the GNU C++/GCC runtimes. A subsequent separate
check found a fixture expectation of 17 characters for a 16-column row; the expected
value was corrected. Checks now exit normally with stderr diagnostics rather than
calling `abort`, and the runner sets a process-local, child-inherited error mode
to keep unattended failures out of desktop dialogs. Replacement tests passed;
these results do not retroactively pass the original failed execution.

## Protocol reference checks

Compared field layouts against the primary
[Wireshark LLDP dissector](https://github.com/wireshark/wireshark/blob/master/epan/dissectors/packet-lldp.c)
and [CDP dissector](https://github.com/wireshark/wireshark/blob/master/epan/dissectors/packet-cdp.c):
LLDP has a seven-bit type/nine-bit length; LLDP-MED voice policy separates policy
flags from VLAN/priority bits; CDP voice VLAN follows a one-byte appliance value.
EtherCard 1.1.0 `packetReceive()` bounds its return to `bufferSize - 1` and checks
the ENC28J60 receive status. The implementation reads only the returned bytes.

Unresolved: on-device memory/stack soak testing, power/wiring verification, LCD
readability/timing, actual switch interoperability, frame tagging support and
neighbor lifetime policy. No unsupported physical-validation claims are made.
