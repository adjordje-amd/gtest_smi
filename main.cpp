
#include <chrono>
#include <iostream>
#include <thread>

#include "smi/amd_smi_driver.hpp"
#include "smi/data_collector.hpp"
#include "smi/service.hpp"

int main() {
  smi::service<smi::amd_smi_driver_factory> service;

  auto processors = service.get_processors();
  auto x = 30;
  while (x) {
    processors[0]->get_data();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    x--;
  }

  return 0;
}
