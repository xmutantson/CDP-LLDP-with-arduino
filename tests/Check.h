#ifndef TEST_CHECK_H
#define TEST_CHECK_H
#include <cstdlib>
#include <iostream>
// Exit normally on failure, keeping unattended tests out of Windows crash dialogs.
#define CHECK(condition) do { \
  if (!(condition)) { \
    std::cerr << __FILE__ << ':' << __LINE__ << ": FAIL " << #condition << '\n'; \
    std::exit(EXIT_FAILURE); \
  } \
} while (false)
#endif
