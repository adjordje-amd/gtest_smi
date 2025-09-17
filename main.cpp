
#include <chrono>
#include <iostream>
#include <thread>

#include "smi/amd_smi_driver.hpp"
#include "smi/data_collector.hpp"
#include "smi/service.hpp"

int main() {
  rocprofsys::amd_smi::service<rocprofsys::amd_smi::amd_smi_driver_factory>
      service;

  auto processors = service.get_processors();
  processors[0]->print_supported_metrics();
  processors[0]->get_smi_metrics();

  return 0;
}
