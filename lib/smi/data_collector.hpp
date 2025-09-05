/**
 * @file data_collector.hpp
 * @brief Defines the smi::data_collector class template for sampling processor
 * metrics.
 *
 * This file provides the data_collector class template, which collects
 * temperature, power, and usage metrics from all available processors using the
 * AMD SMI driver. It is designed to be used within the smi namespace and
 * supports dependency injection of different driver factories.
 */

#pragma once

#include "service.hpp"

#include <cstdint>
#include <iostream>

namespace smi {

/**
 * @struct data_sample
 * @brief Represents a single sample of processor metrics.
 *
 * Contains temperature, power, and usage data for a processor.
 */
struct data_sample {
  int64_t temperature; ///< Processor temperature in millidegrees Celsius
  uint32_t power;      ///< Processor power in milliwatts
  uint32_t usage;      ///< Processor usage (custom metric)
};

/**
 * @class data_collector
 * @tparam driver_factory The factory type used to create the driver interface.
 * @brief Collects metrics from all available processors using the AMD SMI
 * driver.
 *
 * The data_collector class initializes the SMI service, enumerates all
 * processors, and provides a method to read temperature and power metrics for
 * each processor.
 */
template <typename driver_factory> struct data_collector {

  using driver_t = driver_factory::driver_t;

  /**
   * @brief Constructs a data_collector and initializes processor list.
   */
  data_collector() : m_smi(std::make_unique<service<driver_factory>>()) {
    m_processors = m_smi->get_processors();
    std::cout << "Processors size " << m_processors.size() << std::endl;
    m_sample.resize(m_processors.size());
  }

  /**
   * @brief Reads temperature and power metrics from all processors.
   * @return Reference to the vector of data_sample structs.
   * @note If a processor read fails, the error is logged and the sample is not
   * updated.
   */
  const std::vector<data_sample> &read() {
    size_t id{0};
    for (auto &item : m_processors) {
      try {
        m_sample[id++].power = item->get_power_info();
        m_sample[id++].temperature = item->get_temperature_info();
      } catch (std::runtime_error &error) {
        std::cout << "Failed to read info for the processo id %d. Error: "
                  << error.what() << std::endl;
      }
    }
    return m_sample;
  }

private:
  std::vector<data_sample> m_sample; ///< Samples for each processor
  std::unique_ptr<service<driver_factory>> m_smi; ///< SMI service instance
  std::vector<std::shared_ptr<processor<driver_t>>>
      m_processors; ///< List of processors
};

} // namespace smi
