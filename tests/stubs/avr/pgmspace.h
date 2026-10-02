// Host stand-in for AVR program-memory string copying.
#include <string.h>
#define PSTR(value) value
#define strcpy_P(destination, source) strcpy(destination, source)
