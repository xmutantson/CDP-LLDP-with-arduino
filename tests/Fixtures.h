#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H
#include <stdint.h>
#include <string>
#include <vector>

// Synthetic, ordinary advertisements held in memory only. No capture/transmit API.
namespace fixtures {
typedef std::vector<uint8_t> Bytes;
inline Bytes bytes(const std::string& value) { return Bytes(value.begin(), value.end()); }
inline void word(Bytes& out, size_t value) {
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}
inline void cdpTlv(Bytes& out, size_t type, const Bytes& value) {
  word(out, type); word(out, value.size() + 4);
  out.insert(out.end(), value.begin(), value.end());
}
inline Bytes cdp(bool full = true) {
  Bytes out = {1,0,12,204,204,204, 2,0,0,0,0,1, 0,0,
               170,170,3,0,0,12,32,0, 2,120,0,0};
  cdpTlv(out, 1, bytes(full ? "test-switch" : "new"));
  cdpTlv(out, 3, bytes("Gi1/0/24"));
  if (full) {
    cdpTlv(out, 6, bytes("test-model"));
    cdpTlv(out, 10, {0,42});
    cdpTlv(out, 14, {1,1,44}); // Voice VLAN 300.
    cdpTlv(out, 2, {0,0,0,1, 1,1,204, 0,4, 192,0,2,1});
  }
  const size_t length = out.size() - 14;
  out[12] = static_cast<uint8_t>(length >> 8);
  out[13] = static_cast<uint8_t>(length);
  // Populate the CDP checksum so fixtures also represent normal complete messages.
  uint32_t sum = 0;
  for (size_t i = 22; i + 1 < out.size(); i += 2)
    sum += (static_cast<uint16_t>(out[i]) << 8) | out[i + 1];
  if ((out.size() - 22) & 1) sum += out.back(); // CDP's odd-byte convention.
  while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
  const uint16_t checksum = static_cast<uint16_t>(~sum);
  out[24] = checksum >> 8; out[25] = checksum & 255;
  return out;
}
inline void lldpTlv(Bytes& out, size_t type, const Bytes& value) {
  word(out, (type << 9) | value.size());
  out.insert(out.end(), value.begin(), value.end());
}
inline Bytes lldp(bool full = true, bool longDescription = false) {
  Bytes out = {1,128,194,0,0,14, 2,0,0,0,0,2, 136,204};
  lldpTlv(out, 1, {4,2,0,0,0,0,2});
  Bytes port = {5};
  const Bytes name = bytes("Ethernet1/2");
  port.insert(port.end(), name.begin(), name.end());
  lldpTlv(out, 2, port);
  lldpTlv(out, 3, {0,120});
  if (longDescription) lldpTlv(out, 6, bytes(std::string(260, 'D')));
  if (full) {
    lldpTlv(out, 5, bytes("lldp-switch"));
    lldpTlv(out, 127, {0,128,194,1,0,77});
    lldpTlv(out, 127, {0,18,187,10,'m','o','d','e','l'});
    // Defined tagged voice policy, VLAN 300, priority 5, DSCP 46.
    lldpTlv(out, 127, {0,18,187,2,1,0x42,0x59,0x6e});
    lldpTlv(out, 8, {5,1,192,0,2,2, 2,0,0,0,1,0});
  }
  lldpTlv(out, 0, {});
  return out;
}
}
#endif
