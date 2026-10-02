#include "../DiscoveryParser.h"
#include "Fixtures.h"
#include "Check.h"
#include <iostream>

using fixtures::Bytes;
static void same(const char* actual, const char* expected) { CHECK(strcmp(actual, expected) == 0); }
static void parse(const Bytes& bytes, discovery::Fields& out) {
  CHECK(discovery::parse(bytes.data(), bytes.size(), out));
}
int main() {
  discovery::Fields out = {};
  parse(fixtures::cdp(), out);
  CHECK(out.protocol == discovery::CDP);
  same(out.device, "test-switch"); same(out.mac, "020000000001");
  same(out.port, "Gi1/0/24"); same(out.model, "test-model");
  same(out.vlan, "42"); same(out.ip, "192.0.2.1"); same(out.voice, "300");
  std::cout << "PASS CDP fields and multi-byte voice VLAN\n";

  parse(fixtures::lldp(true, true), out);
  CHECK(out.protocol == discovery::LLDP);
  same(out.device, "lldp-switch"); same(out.mac, "020000000002");
  same(out.port, "Ethernet1/2"); same(out.model, "model");
  same(out.vlan, "77"); same(out.ip, "192.0.2.2"); same(out.voice, "300");
  std::cout << "PASS LLDP fields, nine-bit length and voice policy flags\n";

  parse(fixtures::cdp(false), out);
  same(out.device, "new"); same(out.model, ""); same(out.vlan, "");
  same(out.ip, ""); same(out.voice, "");
  parse(fixtures::lldp(false), out);
  same(out.device, ""); same(out.model, ""); same(out.vlan, "");
  same(out.ip, ""); same(out.voice, "");
  std::cout << "PASS consecutive advertisements clear absent fields\n";

  char row[17] = {};
  const Bytes longName = fixtures::bytes("prefix-that-does-not-fit-port-123456789012");
  discovery::text(row, longName.data(), longName.size());
  same(row, "prefix-that-does");
  discovery::text(row, longName.data(), longName.size(), true);
  same(row, "ort-123456789012"); // Checked below against the last 16 bytes.
  std::cout << "PASS fixed-width text formatting\n";

  const Bytes control = {'A',0,'B','\n'};
  discovery::text(row, control.data(), control.size()); same(row, "A?B?");
  discovery::text(row, NULL, 0); same(row, "");
  std::cout << "PASS text termination and LCD character normalization\n";

  for (int kind = 0; kind < 2; ++kind) {
    Bytes normal = kind ? fixtures::lldp() : fixtures::cdp();
    parse(normal, out);
    const discovery::Fields before = out;
    // Model an incomplete receive buffer, without constructing an exploit packet.
    CHECK(!discovery::parse(normal.data(), normal.size() - 1, out));
    CHECK(memcmp(&before, &out, sizeof out) == 0);
    CHECK(!discovery::parse(normal.data(), 13, out));
    CHECK(memcmp(&before, &out, sizeof out) == 0);
    normal.resize(normal.size() + 20, 0); // Normal Ethernet padding.
    parse(normal, out);
    CHECK(memcmp(&before, &out, sizeof out) == 0);
  }
  CHECK(!discovery::parse(NULL, 0, out));
  std::cout << "PASS incomplete receives preserve snapshot; padding ignored\n";

  // All three standard LLDP destination scopes carry the same ordinary message.
  for (uint8_t scope : {0,3,14}) {
    Bytes normal = fixtures::lldp(); normal[5] = scope; parse(normal, out);
  }
  Bytes ipv4Frame(60, 0); ipv4Frame[0] = 2; ipv4Frame[12] = 8;
  const discovery::Fields before = out;
  CHECK(!discovery::parse(ipv4Frame.data(), ipv4Frame.size(), out));
  CHECK(memcmp(&before, &out, sizeof out) == 0);
  std::cout << "PASS LLDP scopes and unrelated Ethernet traffic\n";

  // Bounded address iteration: skip an IPv6 entry, then retain a following IPv4.
  Bytes addresses = {0,0,0,2, 2,8,170,170,3,0,0,0,134,221, 0,16};
  addresses.insert(addresses.end(), 16, 0);
  const Bytes ip = {1,1,204,0,4,192,0,2,9};
  addresses.insert(addresses.end(), ip.begin(), ip.end());
  discovery::Fields mixed = {};
  CHECK(discovery::cdpAddresses(addresses.data(), addresses.size(), mixed));
  same(mixed.ip, "192.0.2.9");
  std::cout << "PASS mixed address families\n";
}
