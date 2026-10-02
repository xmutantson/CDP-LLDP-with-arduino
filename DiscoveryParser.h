#ifndef DISCOVERY_PARSER_H
#define DISCOVERY_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// No heap allocation or Arduino dependencies. Every display value fits a 1602 row.
namespace discovery {
enum Protocol { None, CDP, LLDP };
struct Fields {
  Protocol protocol;
  char device[17], mac[17], port[17], model[17], vlan[17], ip[17], voice[17];
};

inline uint16_t be16(const uint8_t* p) {
  return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

inline void text(char (&out)[17], const uint8_t* p, size_t length,
                 bool tail = false) {
  const size_t count = length < 16 ? length : 16;
  if (tail && length > count) p += length - count;
  for (size_t i = 0; i < count; ++i)
    out[i] = p[i] >= 32 && p[i] <= 126 ? static_cast<char>(p[i]) : '?';
  out[count] = '\0';
}

inline void mac(char (&out)[17], const uint8_t* p) {
  const char hex[] = "0123456789ABCDEF";
  for (size_t i = 0; i < 6; ++i) {
    out[2 * i] = hex[p[i] >> 4];
    out[2 * i + 1] = hex[p[i] & 15];
  }
  out[12] = '\0';
}

inline void number(char (&out)[17], uint16_t value) {
  snprintf(out, sizeof out, "%u", static_cast<unsigned int>(value));
}

inline void ipv4(char (&out)[17], const uint8_t* p) {
  snprintf(out, sizeof out, "%u.%u.%u.%u", static_cast<unsigned int>(p[0]),
           static_cast<unsigned int>(p[1]), static_cast<unsigned int>(p[2]),
           static_cast<unsigned int>(p[3]));
}

inline bool cdpAddresses(const uint8_t* p, size_t length, Fields& out) {
  if (length < 4) return false;
  const uint32_t count = (static_cast<uint32_t>(p[0]) << 24) |
      (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
  size_t offset = 4;
  for (uint32_t i = 0; i < count; ++i) {
    if (length - offset < 2) return false;
    const uint8_t type = p[offset++];
    const uint8_t protocolLength = p[offset++];
    if (protocolLength > length - offset) return false;
    const bool isIPv4 = type == 1 && protocolLength == 1 && p[offset] == 0xcc;
    offset += protocolLength;
    if (length - offset < 2) return false;
    const size_t addressLength = be16(p + offset);
    offset += 2;
    if (addressLength > length - offset) return false;
    if (isIPv4 && addressLength == 4 && !out.ip[0]) ipv4(out.ip, p + offset);
    offset += addressLength;
  }
  return offset == length;
}

inline bool cdp(const uint8_t* frame, size_t size, Fields& out) {
  // 802.3 length excludes Ethernet header and any padding; SNAP + CDP header = 12.
  const uint8_t snap[] = {0xaa, 0xaa, 0x03, 0x00, 0x00, 0x0c, 0x20, 0x00};
  const size_t payload = be16(frame + 12);
  if (payload < 12 || payload > 1500 || payload > size - 14) return false;
  if (memcmp(frame + 14, snap, sizeof snap) != 0) return false;
  if (frame[22] != 1 && frame[22] != 2) return false;
  size_t offset = 26;
  const size_t end = 14 + payload;
  while (offset < end) {
    if (end - offset < 4) return false;
    const uint16_t type = be16(frame + offset);
    const size_t total = be16(frame + offset + 2);
    if (total < 4 || total > end - offset) return false;
    const uint8_t* value = frame + offset + 4;
    const size_t length = total - 4;
    switch (type) {
      case 1: text(out.device, value, length); break;
      case 2: if (!cdpAddresses(value, length, out)) return false; break;
      case 3: text(out.port, value, length, true); break;
      case 6: text(out.model, value, length, true); break;
      case 10:
        if (length != 2) return false;
        number(out.vlan, be16(value));
        break;
      case 14: // VoIP VLAN reply: appliance byte followed by VLAN ID.
        if (length != 3) return false;
        number(out.voice, be16(value + 1));
        break;
      default: break;
    }
    offset += total;
  }
  return true;
}

inline bool lldpManagement(const uint8_t* p, size_t length, Fields& out) {
  if (length < 1) return false;
  const size_t addressLength = p[0]; // Includes the address family byte.
  if (addressLength < 1 || addressLength > length - 1) return false;
  const size_t rest = 1 + addressLength;
  // Interface subtype + interface number + OID length, then OID bytes.
  if (length - rest < 6 || p[rest + 5] != length - rest - 6) return false;
  if (addressLength == 5 && p[1] == 1 && !out.ip[0]) ipv4(out.ip, p + 2);
  return true;
}

inline bool lldpOrganization(const uint8_t* p, size_t length, Fields& out) {
  if (length < 4) return false;
  const uint32_t oui = (static_cast<uint32_t>(p[0]) << 16) |
      (static_cast<uint32_t>(p[1]) << 8) | p[2];
  if (oui == 0x0080c2 && p[3] == 1) {
    if (length != 6) return false;
    number(out.vlan, be16(p + 4));
  } else if (oui == 0x0012bb && p[3] == 10) {
    text(out.model, p + 4, length - 4, true);
  } else if (oui == 0x0012bb && p[3] == 2) {
    if (length != 8) return false;
    // Only a defined, tagged voice policy has an advertised voice VLAN ID.
    if (p[4] == 1 && (p[5] & 0xc0) == 0x40)
      number(out.voice, (be16(p + 5) >> 1) & 0x0fff);
  }
  return true;
}

inline bool lldp(const uint8_t* frame, size_t size, Fields& out) {
  if (be16(frame + 12) != 0x88cc) return false;
  size_t offset = 14;
  uint8_t mandatory = 1;
  while (offset < size) {
    if (size - offset < 2) return false;
    const uint16_t header = be16(frame + offset);
    const uint8_t type = header >> 9;
    const size_t length = header & 0x01ff;
    offset += 2;
    if (length > size - offset) return false;
    const uint8_t* value = frame + offset;
    if (mandatory <= 3) {
      if (type != mandatory) return false;
      ++mandatory;
    } else if (type >= 1 && type <= 3) {
      return false;
    }
    switch (type) {
      case 0: return length == 0 && mandatory == 4; // Ignore Ethernet padding.
      case 1:
        if (length < 2) return false;
        break;
      case 2:
        if (length < 2) return false;
        if (value[0] == 3 && length == 7) mac(out.port, value + 1);
        else if (value[0] == 4 && length == 6 && value[1] == 1)
          ipv4(out.port, value + 2);
        else if (value[0] == 1 || value[0] == 2 || value[0] == 5 || value[0] == 7)
          text(out.port, value + 1, length - 1, true);
        break;
      case 3: if (length != 2) return false; break;
      case 5: text(out.device, value, length); break;
      case 8: if (!lldpManagement(value, length, out)) return false; break;
      case 127: if (!lldpOrganization(value, length, out)) return false; break;
      default: break;
    }
    offset += length;
  }
  return false; // The complete LLDPDU must contain its end marker.
}

// On incomplete/unsupported input leave the last successfully parsed snapshot intact.
// The caller must pass the number of bytes actually present, never a wire length.
inline bool parse(const uint8_t* frame, size_t size, Fields& out) {
  if (!frame || size < 14) return false;
  const uint8_t cdpDestination[] = {1, 0, 0x0c, 0xcc, 0xcc, 0xcc};
  const uint8_t lldpPrefix[] = {1, 0x80, 0xc2, 0, 0};
  Fields next = {};
  if (memcmp(frame, cdpDestination, 6) == 0) {
    next.protocol = CDP;
    if (!cdp(frame, size, next)) return false;
  } else if (memcmp(frame, lldpPrefix, 5) == 0 &&
             (frame[5] == 0x0e || frame[5] == 0x03 || frame[5] == 0x00)) {
    next.protocol = LLDP;
    if (!lldp(frame, size, next)) return false;
  } else return false;
  mac(next.mac, frame + 6);
  out = next;
  return true;
}
} // namespace discovery
#endif
