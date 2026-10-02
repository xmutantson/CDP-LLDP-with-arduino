#ifndef TEST_ETHERCARD_H
#define TEST_ETHERCARD_H
#include <stdint.h>
#include <string.h>
#include <vector>
typedef uint8_t byte;
typedef uint16_t word;
const int A0=14, A1=15, A2=16, A3=17, A4=18, A5=19, OUTPUT=1;
#define F(value) value
inline void pinMode(int, int) {}
inline void delay(unsigned long) {}
inline unsigned long millis() { return 0; }
struct SerialStub {
  void begin(int) {}
  void print(const char*) {}
  void println(const char*) {}
};
static SerialStub Serial;
class Ethernet { public: static byte buffer[]; };
class ENC28J60 {
 public:
  static bool promiscuous;
  static void enablePromiscuous() { promiscuous = true; }
};
bool ENC28J60::promiscuous = false;
struct EtherStub {
  byte myip[4] = {192,0,2,10}, gwip[4] = {192,0,2,254};
  bool hardware = true, dhcp = true;
  int dhcpCalls = 0, receiveCalls = 0;
  std::vector<byte> pending;
  byte begin(word, const byte*, byte) { return hardware; }
  bool dhcpSetup() { ++dhcpCalls; return dhcp; }
  word packetReceive() {
    ++receiveCalls;
    const word length = static_cast<word>(pending.size());
    if (length < 500 && length) memcpy(Ethernet::buffer, pending.data(), length);
    pending.clear(); return length;
  }
};
static EtherStub ether;
#endif
