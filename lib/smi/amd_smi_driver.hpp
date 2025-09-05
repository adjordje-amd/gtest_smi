#pragma once

#include <amd_smi/amdsmi.h>
#include <cstdint>
#include <iostream>
#include <memory>

namespace smi {

struct amd_smi_driver {
  amdsmi_status_t init(uint64_t init_flags = AMDSMI_INIT_AMD_GPUS) {
    std::cout << "[Driver] amdsmi_init" << std::endl;
    return amdsmi_init(init_flags);
  }

  amdsmi_status_t get_version(amdsmi_version_t *version) {
    return amdsmi_get_lib_version(version);
  }

  amdsmi_status_t get_power_info(amdsmi_processor_handle processor_handle,
                                 amdsmi_power_info_t *info) {
    std::cout << "[Driver] amdsmi_get_power_info" << std::endl;
    return amdsmi_get_power_info(processor_handle, info);
  }

  amdsmi_status_t
  get_temperature_metric(amdsmi_processor_handle processor_handle,
                         amdsmi_temperature_type_t sensor_type,
                         amdsmi_temperature_metric_t metric,
                         int64_t *temperature) {
    std::cout << "[Driver] amdsmi_get_temp_metric" << std::endl;
    return amdsmi_get_temp_metric(processor_handle, sensor_type, metric,
                                  temperature);
  }

  amdsmi_status_t get_socket_handles(uint32_t *socket_count,
                                     amdsmi_socket_handle *socket_handles) {
    std::cout << "[Driver] amdsmi_get_socket_handles" << std::endl;
    return amdsmi_get_socket_handles(socket_count, socket_handles);
  }

  amdsmi_status_t
  get_processor_handles(amdsmi_socket_handle socket_handle,
                        uint32_t *processor_count,
                        amdsmi_processor_handle *processor_handles) {
    std::cout << "[Driver] amdsmi_get_processor_handles" << std::endl;
    return amdsmi_get_processor_handles(socket_handle, processor_count,
                                        processor_handles);
  }

  amdsmi_status_t get_processor_type(amdsmi_processor_handle processor_handle,
                                     processor_type_t *processor_type) {
    std::cout << "[Driver] amdsmi_get_processor_type" << std::endl;
    return amdsmi_get_processor_type(processor_handle, processor_type);
  }
};

struct amd_smi_driver_factory {
  using driver_t = amd_smi_driver;

  static std::shared_ptr<driver_t> create_driver() {
    return std::make_shared<driver_t>();
  }
};

} // namespace smi
