#include <iostream>
#include "json.h"

int main() {
  std::string s;
  std::cin >> s;
  auto v = json::parse(s);
  std::cout << v << "\n";
}


