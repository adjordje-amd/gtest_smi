
#include <iostream>

#include "smi/amd_smi_driver.hpp"
#include "smi/data_collector.hpp"

int main() {
  smi::data_collector<smi::amd_smi_driver_factory> _collector;

  auto data = _collector.read();
  std::cout << "Data size " << data.size() << std::endl;
  for (auto &item : data) {
    std::cout << "***************************************\n";
    std::cout << "Power: " << item.power << "W\n";
    std::cout << "Temperature: " << item.temperature << "C\n";
    std::cout << "***************************************" << std::endl;
  }

  return 0;
}
