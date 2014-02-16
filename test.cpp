#include <iostream>
#include <fstream>
#include "json.h"

int main() {
  auto v = json::parse(std::cin);
  std::cout << v << "\n";
}


