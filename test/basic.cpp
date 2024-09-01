#include <iostream>
#include <sstream>

#include "cron.hpp"

bool test_01() {
  cronio::cron cron;
  std::istringstream{"* * * * *"} >> cron;
  std::tm in = {59, 59, 23, 29, 10 - 1, 1973 - 1900, 0, 0, true};
  std::tm out = {0, 0, 0, 30, 10 - 1, 1973 - 1900, 0, 0, true};
  in = cron.next(in);
  if (std::mktime(&in) == std::mktime(&out)) {
    return true;
  } else {
    std::cerr << "Test 01 failed." << std::endl;
    return false;
  }
}

bool test_02() {
  cronio::cron cron;
  std::istringstream{"10,15 * 20-25 * *"} >> cron;
  std::tm in = {45, 30, 14, 7, 8 - 1, 1984 - 1900, 0, 0, true};
  std::tm out = {0, 10, 0, 20, 8 - 1, 1984 - 1900, 0, 0, true};
  in = cron.next(in);
  if (std::mktime(&in) == std::mktime(&out)) {
    return true;
  } else {
    std::cerr << "Test 02 failed." << std::endl;
    return false;
  }
}

bool test_03() {
  cronio::cron cron;
  std::istringstream{"11,15-20 16-18,20-22 * 3-9 0,6"} >> cron;
  std::tm in = {0, 0, 8, 12, 6 - 1, 2020 - 1900, 0, 0, true};
  std::tm out = {0, 11, 16, 13, 6 - 1, 2020 - 1900, 0, 0, true};
  in = cron.next(in);
  if (std::mktime(&in) == std::mktime(&out)) {
    return true;
  } else {
    std::cerr << "Test 03 failed." << std::endl;
    return false;
  }
}

int main(int argc, const char* argv[]) {
  if (              //
      test_01() &&  //
      test_02() &&  //
      test_03() &&  //
      true) {
    return 0;
  }
  return -1;
}
