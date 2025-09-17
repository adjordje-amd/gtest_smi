// Copyright (c) 2018-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// with the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
//
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimers in the
// documentation and/or other materials provided with the distribution.
//
// * Neither the names of Advanced Micro Devices, Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this Software without specific prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.

#pragma once

#include <algorithm>
#include <amd_smi/amdsmi.h>
#include <bitset>
#include <cstdint>

#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace rocprofsys {
namespace amd_smi {

constexpr auto metric_value_not_supported = 0xffff;

struct supported_metrics {
  uint32_t current_socket_power : 1;
  uint32_t average_socket_power : 1;
  uint32_t memory_usage : 1;
  uint32_t temperature : 1;
  uint32_t gfx_activity : 1;
  uint32_t umc_activity : 1;
  uint32_t mm_activity : 1;
  uint32_t vcn_xcp_stats : 1;
  uint32_t jpeg_xcp_stats : 1;
  std::bitset<AMDSMI_MAX_NUM_VCN> vcn_activity_engine;
  std::bitset<AMDSMI_MAX_NUM_JPEG_ENG_V1> jpeg_activity_engine;
};

struct smi_metrics {
  uint32_t current_socket_power;
  uint32_t average_socket_power;
  uint32_t memory_usage;
  uint16_t temperature;
  uint32_t gfx_activity;
  uint32_t umc_activity;
  uint32_t mm_activity;
  uint16_t vcn_activity[AMDSMI_MAX_NUM_VCN];
  uint16_t jpeg_activity[AMDSMI_MAX_NUM_JPEG_ENG_V1];
};

template <typename BitsetT>
static std::string bitset_to_index_list(const BitsetT &bs) {
  std::stringstream ss;
  bool first = true;
  ss << std::boolalpha << "[";
  for (std::size_t i = 0; i < bs.size(); ++i) {
    ss << (bool)bs[i];
    if (i != bs.size() - 1) {
      ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

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

  supported_metrics get_supported_metrics() {
    static auto fetch_supported_metrics = [&] {
      amdsmi_power_info_t socker_power_info;
      auto driver_call_result =
          m_driver_api->get_power_info(m_processor_handle, &socker_power_info);

      m_supported_metrics.average_socket_power =
          driver_call_result == AMDSMI_STATUS_SUCCESS &&
          socker_power_info.average_socket_power != metric_value_not_supported;
      m_supported_metrics.current_socket_power =
          driver_call_result == AMDSMI_STATUS_SUCCESS &&
          socker_power_info.current_socket_power != metric_value_not_supported;

      amdsmi_engine_usage_t info;
      driver_call_result =
          m_driver_api->get_gpu_activity(m_processor_handle, &info);
      m_supported_metrics.gfx_activity =
          driver_call_result == AMDSMI_STATUS_SUCCESS;
      m_supported_metrics.mm_activity =
          driver_call_result == AMDSMI_STATUS_SUCCESS;
      m_supported_metrics.umc_activity =
          driver_call_result == AMDSMI_STATUS_SUCCESS;

      uint64_t memory_usage;
      driver_call_result = m_driver_api->get_memory_usage(
          m_processor_handle, AMDSMI_MEM_TYPE_VRAM, &memory_usage);
      m_supported_metrics.memory_usage =
          driver_call_result == AMDSMI_STATUS_SUCCESS;

      int64_t temperature;
      driver_call_result = m_driver_api->get_temperature_metric(
          m_processor_handle, AMDSMI_TEMPERATURE_TYPE_JUNCTION,
          AMDSMI_TEMP_CURRENT, &temperature);
      m_supported_metrics.temperature =
          driver_call_result == AMDSMI_STATUS_SUCCESS;

      amdsmi_gpu_metrics_t gpu_metrics;
      driver_call_result =
          m_driver_api->get_gpu_metrics_info(m_processor_handle, &gpu_metrics);
      std::for_each(std::begin(gpu_metrics.vcn_activity),
                    std::end(gpu_metrics.vcn_activity),
                    [&, index = 0](const auto val) mutable {
                      m_supported_metrics.vcn_activity_engine[index++] =
                          driver_call_result == AMDSMI_STATUS_SUCCESS &&
                          val != metric_value_not_supported;
                    });
      std::for_each(std::begin(gpu_metrics.jpeg_activity),
                    std::end(gpu_metrics.jpeg_activity),
                    [&, index = 0](const auto val) mutable {
                      m_supported_metrics.jpeg_activity_engine[index++] =
                          driver_call_result == AMDSMI_STATUS_SUCCESS &&
                          val != metric_value_not_supported;
                    });

      m_supported_metrics.jpeg_xcp_stats = std::any_of(
          std::begin(gpu_metrics.xcp_stats), std::end(gpu_metrics.xcp_stats),
          [](const amdsmi_gpu_xcp_metrics_t &xcp_stats) {
            return std::any_of(std::begin(xcp_stats.jpeg_busy),
                               std::end(xcp_stats.jpeg_busy),
                               [index = 0](const auto &item) mutable {
                                 std::cout << std::dec << index++
                                           << ". XCP JPEG Engine " << std::hex
                                           << item << std::endl;
                                 return item != metric_value_not_supported;
                               });
          });
      m_supported_metrics.vcn_xcp_stats = std::any_of(
          std::begin(gpu_metrics.xcp_stats), std::end(gpu_metrics.xcp_stats),
          [&](const amdsmi_gpu_xcp_metrics_t &xcp_stats) {
            return std::any_of(
                std::begin(xcp_stats.vcn_busy), std::end(xcp_stats.vcn_busy),
                [&, index = 0](const auto &item) mutable {
                  std::cout << std::dec << index++ << ". XCP VCN Engine "
                            << std::hex << item << ", VCN Engine"
                            << gpu_metrics.vcn_activity[0] << std::endl;
                  return item != metric_value_not_supported;
                });
          });
      return true;
    }();

    return m_supported_metrics;
  }

  /**
   * @brief Returns the type of the processor.
   * @return The processor type.
   */
  processor_type_t get_processor_type() { return m_processor_type; }

  smi_metrics get_smi_metrics() {
    amdsmi_gpu_metrics_t gpu_metrics;
    auto driver_call_result =
        m_driver_api->get_gpu_metrics_info(m_processor_handle, &gpu_metrics);
    if (driver_call_result != AMDSMI_STATUS_SUCCESS) {
      throw std::runtime_error("Failed to read SMI data! AMD SMI Error code: " +
                               std::to_string(driver_call_result));
    }

    uint64_t memory_usage = std::numeric_limits<uint64_t>::max();
    driver_call_result = m_driver_api->get_gpu_memory_usage(
        m_processor_handle, AMDSMI_MEM_TYPE_VRAM, &memory_usage);
    if (driver_call_result != AMDSMI_STATUS_SUCCESS) {
      std::cout << "Failed to read SMI data! AMD SMI Error code: "
                << driver_call_result << std::endl;
    }

    auto populate_metrics = [](auto flag, const auto &source,
                               auto &destination) {
      if (flag)
        destination = source;
    };

    smi_metrics metrics;

    populate_metrics(m_supported_metrics.average_socket_power,
                     gpu_metrics.average_socket_power,
                     metrics.average_socket_power);
    populate_metrics(m_supported_metrics.current_socket_power,
                     gpu_metrics.current_socket_power,
                     metrics.current_socket_power);
    populate_metrics(m_supported_metrics.memory_usage, memory_usage,
                     metrics.memory_usage);
    populate_metrics(m_supported_metrics.gfx_activity,
                     gpu_metrics.average_gfx_activity, metrics.gfx_activity);
    populate_metrics(m_supported_metrics.umc_activity,
                     gpu_metrics.average_umc_activity, metrics.umc_activity);
    populate_metrics(m_supported_metrics.mm_activity,
                     gpu_metrics.average_mm_activity, metrics.mm_activity);
    populate_metrics(m_supported_metrics.temperature,
                     gpu_metrics.temperature_hotspot, metrics.temperature);

    // uint32_t vcn_activity  : 1;
    // uint32_t jpeg_activity : 1;

    return metrics;
  }

  void print_supported_metrics() {
    auto metrics = get_supported_metrics();

    std::cout << "=== SUPPORTED SMI METRICS ===" << '\n';
    std::cout << std::left << std::boolalpha;

    // Power metrics
    std::cout << "  " << std::setw(25) << "current_socket_power"
              << ": " << (bool)metrics.current_socket_power << '\n';
    std::cout << "  " << std::setw(25) << "average_socket_power"
              << ": " << (bool)metrics.average_socket_power << '\n';

    // Memory and thermal metrics
    std::cout << "  " << std::setw(25) << "memory_usage"
              << ": " << (bool)metrics.memory_usage << '\n';
    std::cout << "  " << std::setw(25) << "temperature"
              << ": " << (bool)metrics.temperature << '\n';

    std::cout << "  " << std::setw(25) << "gfx_activity"
              << ": " << (bool)metrics.gfx_activity << '\n';
    std::cout << "  " << std::setw(25) << "umc_activity"
              << ": " << (bool)metrics.umc_activity << '\n';
    std::cout << "  " << std::setw(25) << "mm_activity"
              << ": " << (bool)metrics.mm_activity << '\n';

    std::cout << "  " << std::setw(25) << "vcn_xcp_stats" << ": "
              << (bool)metrics.vcn_xcp_stats << '\n';
    std::cout << "  " << std::setw(25) << "vcn_activity_engine"
              << ": " << bitset_to_index_list(metrics.vcn_activity_engine)
              << '\n';
    std::cout << "  " << std::setw(25) << "jpeg_xcp_stats" << ": "
              << (bool)metrics.jpeg_xcp_stats << '\n';
    std::cout << "  " << std::setw(25) << "jpeg_activity_engine"
              << ": " << bitset_to_index_list(metrics.jpeg_activity_engine)
              << '\n';

    std::cout << "=========================" << std::endl;
  }

private:
  supported_metrics m_supported_metrics;
  std::shared_ptr<driver> m_driver_api;
  amdsmi_processor_handle m_processor_handle;
  processor_type_t m_processor_type;
};

} // namespace amd_smi
} // namespace rocprofsys
