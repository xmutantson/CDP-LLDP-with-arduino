#include <EtherCard.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include "DiscoveryParser.h"

// 1602A adaptation of ModLogNet's ENC28J60 listener, based on qistoph/CdpSniffino.
// See README.md for wiring, fixed-memory limits and offline build/test instructions.
byte mymac[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
const int rs = A5, en = A4, d4 = A0, d5 = A1, d6 = A2, d7 = A3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte Ethernet::buffer[500];
bool ethernetReady = false;
discovery::Fields neighbor = {};
unsigned long previousMillis = 0;
const unsigned long interval = 250;
unsigned long lcdCount = 0;
unsigned long LCD_Window_control = 20;

char W1L1[17] = "", W1L2[17] = "";
char W2L1[17] = "", W2L2[17] = "";
char W3L1[17] = "Device", W3L2[17] = "";
char W4L1[17] = "SwDevMAC:", W4L2[17] = "";
char W5L1[17] = "Port:", W5L2[17] = "";
char W6L1[17] = "Model:", W6L2[17] = "";
char W7L1[17] = "VLAN:", W7L2[17] = "";

void LCDupdate();
void updateNeighborWindows();

void setup()
{
//speaking of ram usage, if you ever print something static you should use a flash string. Serial.print(F("something"));
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  Serial.println(F("hello!"));
  Serial.println(F("init.."));
  //init lcd and display a starting sequence message. have to do it manually because the dynamic loop cant be called yet
  lcd.begin(16, 2);

  lcd.print(F("Arduino Ethernet"));
  lcd.setCursor(0, 1);
  lcd.print(F("CDP/LLDP reader "));
  delay(1500); //somewhat arbitrary, to allow you to actually read the first window
  lcd.setCursor(0, 0);
  lcd.print(F("Cred: Xmutantson"));
  lcd.setCursor(0, 1);
  lcd.print(F("&ModLogNetgithub"));
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print(F("CDP Takes `60sec"));
  lcd.setCursor(0, 1);
  lcd.print(F("to be detected! "));
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print(F("Waiting for IP  "));
  lcd.setCursor(0, 1);
  lcd.print(F("from DHCP server"));
  ethernetReady = ether.begin(sizeof Ethernet::buffer, mymac, 10) != 0;
  if (!ethernetReady) {
    strcpy_P(W1L1, PSTR("ENC28J60 failed"));
    strcpy_P(W1L2, PSTR("Check SPI wiring"));
    LCD_Window_control = 10;
    return;
  }

  if (!ether.dhcpSetup())
  {

    strcpy_P(W1L1, PSTR("DHCP failed....."));
    strcpy_P(W1L2, PSTR(" Restart tool or"));
    strcpy_P(W2L1, PSTR(" Restart tool or"));
    strcpy_P(W2L2, PSTR(" check DHCP svr "));
    strcpy_P(W3L1, PSTR(" check DHCP svr "));
    strcpy_P(W3L2, PSTR(" or port wiring."));
    LCD_Window_control = 30;
    Serial.println(F("DHCP FailWHACK!"));
  }
  else
  {
    //    testdrawtext("DHCP IP:",ST7735_GREEN);
    strcpy_P(W1L1, PSTR("My DHCP IP:     "));
    Serial.println(W1L1);
    discovery::ipv4(W1L2, ether.myip); //convert the bytearray ether.myip to char and write to W1L2
    Serial.println(W1L2); //print what's going to this spot in the display window/line cycle
    Serial.println(F("DHCP IP OBTAINED!"));  //serial print ack of getting dhcp working

    delay(1000);
    LCD_Window_control = 20;

    // ether.printIp("GW: ", ether.gwip);
    strcpy_P(W2L1, PSTR("Gateway IP:     "));
    Serial.println(W2L1);
    discovery::ipv4(W2L2, ether.gwip); //convert the bytearray ether.myip to char and write to W1L2
    Serial.println(W2L2); //print what's going to this spot in the display window/line cycle
    Serial.println(F("Gateway IP OBTAINED!"));  //serial print ack of getting dhcp working
  }
  // Discovery is layer 2 and still works when DHCP is unavailable.
  ENC28J60::enablePromiscuous();
}
void loop()
{

  unsigned long currentMillis = millis();   // Update the LCD every 250 ms.
  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    LCDupdate();                            //call lcdupdate
    if (lcdCount >= 10000) {                // Bound the display counter.
      lcdCount = 0;
    }
    lcdCount++;                         //increment lcdCount to tell program how many times we've pulsed the lcd
    //Serial.println("lcdWHACK!");      //state machine indicator
    //Serial.println(lcdCount);         //lcdCount indicator
  }

  if (!ethernetReady) return;
  const word plen = ether.packetReceive();
  // EtherCard reserves one buffer byte; never pass an unbounded wire length.
  if (plen > 0 && plen < sizeof Ethernet::buffer &&
      discovery::parse(Ethernet::buffer, plen, neighbor)) {
    updateNeighborWindows();
  }
}

void updateNeighborWindows() {
  strcpy_P(W3L1, neighbor.protocol == discovery::CDP ? PSTR("CDPDevID:") : PSTR("LLDP System:"));
  strcpy_P(W4L1, neighbor.protocol == discovery::CDP ? PSTR("CDPDevMAC:") : PSTR("LLDP Src MAC:"));
  strcpy_P(W5L1, PSTR("Port ID: (trim)"));
  strcpy_P(W6L1, PSTR("Model No: (trim)"));
  strcpy_P(W7L1, PSTR("VLAN ID:"));
  memcpy(W3L2, neighbor.device, sizeof W3L2);
  memcpy(W4L2, neighbor.mac, sizeof W4L2);
  memcpy(W5L2, neighbor.port, sizeof W5L2);
  memcpy(W6L2, neighbor.model, sizeof W6L2);
  memcpy(W7L2, neighbor.vlan, sizeof W7L2);
  LCD_Window_control = 70;
  // Retain the seven-window cycle; management/voice data remains available on serial.
  Serial.println(W3L1);
  Serial.println(neighbor.device);
  Serial.println(neighbor.mac);
  Serial.println(neighbor.port);
  Serial.println(neighbor.model);
  Serial.println(neighbor.vlan);
  Serial.print(F("Management IPv4: "));
  Serial.println(neighbor.ip);
  Serial.print(F("Voice VLAN: "));
  Serial.println(neighbor.voice);
}

void LCDupdate () {  //updates display, sequentially. I know it's bad but this works for now until we need it to be better

  if (0 < lcdCount && lcdCount <= 10) {
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W1L1); //WINDOW 1 LINE 1
    lcd.setCursor(0, 1);
    lcd.print(W1L2); //WINDOW 1 LINE 2
  }
  if (10 < lcdCount && lcdCount <= 20) {
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W2L1);
    lcd.setCursor(0, 1);
    lcd.print(W2L2);
  }
  if (20 < lcdCount && lcdCount <= 30) {
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W3L1);
    lcd.setCursor(0, 1);
    lcd.print(W3L2);
  }
  if (30 < lcdCount && lcdCount <= 40) {
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W4L1);
    lcd.setCursor(0, 1);
    lcd.print(W4L2);
  }
  if (40 < lcdCount && lcdCount <= 50) {
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W5L1);
    lcd.setCursor(0, 1);
    lcd.print(W5L2);
  }
  if (50 < lcdCount && lcdCount <= 60) {
    //lcd.noAutoscroll();
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W6L1);
    lcd.setCursor(0, 1);
    lcd.print(W6L2);

  }
  if (60 < lcdCount && lcdCount <= 70) {
    //lcd.autoscroll();
    lcd.setCursor(0, 0);
    lcd.print(F("                ")); //WINDOW 1 LINE 1 CLEARING
    lcd.setCursor(0, 1);
    lcd.print(F("                ")); //WINDOW 1 LINE 2 CLEARING
    lcd.setCursor(0, 0);
    lcd.print(W7L1);
    lcd.setCursor(0, 1);
    lcd.print(W7L2);
  }
  if (lcdCount > LCD_Window_control ) { //reset scrollers
    lcdCount = 0;
  }
}
