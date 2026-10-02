#ifndef TEST_LCD_H
#define TEST_LCD_H
#include <string.h>
class LiquidCrystal {
 public:
  char rows[2][17] = {};
  int column = 0, row = 0;
  LiquidCrystal(int, int, int, int, int, int) {}
  void begin(int, int) { memset(rows, 0, sizeof rows); }
  void setCursor(int c, int r) { column=c; row=r; }
  void print(const char* value) {
    while (*value && column < 16) rows[row][column++] = *value++;
  }
};
#endif
