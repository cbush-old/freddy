#include <iostream>
#include "json.hpp"

int main() {
  auto v = json::parse(std::cin);
  std::cout << v << "\n";
}


