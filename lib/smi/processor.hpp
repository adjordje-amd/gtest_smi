/**
 * @file processor.hpp
 * @brief Defines the smi::processor class template for interacting with AMD SMI
 * processors.
 *
 * This file provides the processor class template, which acts as a wrapper for
 * processor-related operations using a provided driver interface. It allows
 * querying processor type, power information, and temperature metrics. The
 * class is designed to be used within the smi namespace and supports dependency
 * injection of different driver implementations.
 */

#pragma once

#include "common.hpp"

namespace smi {

/**
 * @class processor
 * @tparam driver The driver interface type used to communicate with the
 * processor.
 * @brief Encapsulates operations for a single AMD SMI processor.
 *
 * The processor class provides methods to query processor type, power
 * information, and temperature metrics. It is constructed with a shared pointer
 * to a driver, a processor handle, and the processor type.
 */
template <typename driver> struct processor {
  /**
   * @brief Constructs a processor object.
   * @param _driver Shared pointer to the driver interface.
   * @param handle The processor handle.
   * @param processor_type The type of the processor.
   */
  processor(std::shared_ptr<driver> _driver, amdsmi_processor_handle handle,
            processor_type_t processor_type)
      : m_driver_api{_driver}, m_processor_handle{handle},
        m_processor_type{processor_type} {}

  /**
   * @brief Returns the type of the processor.
   * @return The processor type.
   */
  processor_type_t get_processor_type() { return m_processor_type; }

  /**
   * @brief Retrieves the average socket power for the processor.
   * @return The average socket power in milliwatts.
   * @throws std::runtime_error if the driver call fails.
   */
  uint32_t get_power_info() {
    amdsmi_power_info_t power_info;
    check_status(m_driver_api->get_power_info(m_processor_handle, &power_info),
                 "Failed to get device power info!");

    return power_info.average_socket_power;
  }

  /**
   * @brief Retrieves the current junction temperature for the processor.
   * @return The current temperature in millidegrees Celsius.
   * @throws std::runtime_error if the driver call fails.
   */
  int64_t get_temperature_info() {
    int64_t temperature;
    check_status(m_driver_api->get_temperature_metric(
                     m_processor_handle, AMDSMI_TEMPERATURE_TYPE_JUNCTION,
                     AMDSMI_TEMP_CURRENT, &temperature),
                 "Failed to get device power info!");

    return temperature;
  }

private:
  /** Shared pointer to the driver interface used for processor operations. */
  std::shared_ptr<driver> m_driver_api;
  /** Handle to the processor. */
  amdsmi_processor_handle m_processor_handle;
  /** Type of the processor. */
  processor_type_t m_processor_type;
};

} // namespace smi
