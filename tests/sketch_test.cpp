#include "../LCD_Listener_Nano.ino"
#include "Fixtures.h"
#include "Check.h"
#include <iostream>

int main() {
  ether.hardware = false;
  setup(); loop();
  CHECK(!ethernetReady && ether.dhcpCalls == 0 && ether.receiveCalls == 0);
  CHECK(LCD_Window_control == 10);
  std::cout << "PASS controller failure stops receive and DHCP\n";

  ether.hardware = true; ether.dhcp = false;
  setup();
  CHECK(ethernetReady && ENC28J60::promiscuous && LCD_Window_control == 30);
  ether.pending = fixtures::cdp(); loop();
  CHECK(strcmp(W3L2, "test-switch") == 0 && strcmp(W7L2, "42") == 0);
  CHECK(LCD_Window_control == 70);
  std::cout << "PASS CDP display after DHCP failure\n";

  ether.pending = fixtures::lldp(); loop();
  CHECK(strcmp(W3L1, "LLDP System:") == 0);
  CHECK(strcmp(W3L2, "lldp-switch") == 0);
  CHECK(strcmp(W4L2, "020000000002") == 0);
  CHECK(strcmp(W5L2, "Ethernet1/2") == 0);
  CHECK(strcmp(W6L2, "model") == 0 && strcmp(W7L2, "77") == 0);
  lcdCount = 21; LCDupdate();
  CHECK(strcmp(lcd.rows[1], "lldp-switch     ") == 0);
  ether.pending = fixtures::lldp(false); loop(); LCDupdate();
  CHECK(strcmp(lcd.rows[1], "                ") == 0);
  CHECK(!W6L2[0] && !W7L2[0]);
  std::cout << "PASS LLDP display and stale LCD row clearing\n";

  ether.pending = fixtures::cdp(); loop();
  ether.pending = fixtures::lldp(); ether.pending.pop_back(); loop();
  CHECK(strcmp(W3L2, "test-switch") == 0);
  ether.pending.resize(500); loop();
  CHECK(strcmp(W3L2, "test-switch") == 0);
  std::cout << "PASS incomplete receives do not replace display\n";

  ether.dhcp = true; setup();
  CHECK(LCD_Window_control == 20);
  CHECK(strcmp(W1L2, "192.0.2.10") == 0);
  CHECK(strcmp(W2L2, "192.0.2.254") == 0);
  std::cout << "PASS DHCP IP formatting and initial window count\n";
}
