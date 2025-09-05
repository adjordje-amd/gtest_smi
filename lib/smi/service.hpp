/**
 * @file service.hpp
 * @brief Defines the smi::service class template for managing AMD SMI driver
 * and processors.
 *
 * This file provides the service class template, which acts as a high-level
 * interface for initializing the AMD SMI driver, retrieving driver version
 * information, and enumerating available processors. The class is designed to
 * be used within the smi namespace and supports dependency injection of
 * different driver factory implementations.
 */

#pragma once

#include "common.hpp"
#include "processor.hpp"

#include <amd_smi/amdsmi.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace smi {

/**
 * @class service
 * @tparam driver_factory The factory type used to create the driver interface.
 * @brief Manages AMD SMI driver initialization and processor enumeration.
 *
 * The service class provides methods to initialize the AMD SMI driver, retrieve
 * its version, and enumerate all available processors. It uses a driver factory
 * to create the driver instance and wraps lower-level driver calls with error
 * checking.
 */
template <typename driver_factory> struct service {

  using driver_t = typename driver_factory::driver_t;

  /**
   * @brief Constructs a service object and initializes the AMD SMI driver.
   * @throws std::runtime_error if driver initialization or version retrieval
   * fails.
   */
  service() : m_driver_api(driver_factory::create_driver()) {
    check_status(m_driver_api->init(), "Fail to initialize AMD SMI driver!");

    amdsmi_version_t version;
    check_status(m_driver_api->get_version(&version),
                 "Fail to get AMD SMI driver version!");
    m_version = {.numeric_representation = {.major = version.major,
                                            .minor = version.minor,
                                            .release = version.release},
                 .string_representation = std::string{version.build}};
  }

  /**
   * @brief Returns the AMD SMI driver version information.
   * @return Reference to the version struct.
   */
  const version &get_version() { return m_version; }

  /**
   * @brief Enumerates all available processors managed by the AMD SMI driver.
   * @return Vector of shared pointers to processor objects.
   * @throws std::runtime_error if processor enumeration fails.
   */
  std::vector<std::shared_ptr<processor<driver_t>>> get_processors() {
    std::vector<std::shared_ptr<processor<driver_t>>> processors{};
    auto socket_handles = get_socket_handles();

    for (auto &socket_handle : socket_handles) {
      auto processor_handles = get_processor_handles(socket_handle);
      for (auto &processor_handle : processor_handles) {
        processor_type_t processor_type;
        check_status(
            m_driver_api->get_processor_type(processor_handle, &processor_type),
            "Failed to get processor type!");
        // We can implement filter later
        processors.emplace_back(std::make_shared<processor<driver_t>>(
            m_driver_api, processor_handle, processor_type));
      }
    };
    return processors;
  }

private:
  /**
   * @brief Retrieves all socket handles from the AMD SMI driver.
   * @return Vector of socket handles.
   * @throws std::runtime_error if socket handle retrieval fails.
   */
  std::vector<amdsmi_socket_handle> get_socket_handles() {
    uint32_t count;
    check_status(m_driver_api->get_socket_handles(&count, nullptr),
                 "Failed to get socket handles!");
    std::vector<amdsmi_socket_handle> socket_handles(count);
    check_status(
        m_driver_api->get_socket_handles(&count, socket_handles.data()),
        "Failed to get socket handles!");
    return socket_handles;
  }

  /**
   * @brief Retrieves all processor handles for a given socket.
   * @param socket_handle The socket handle to query.
   * @return Vector of processor handles.
   * @throws std::runtime_error if processor handle retrieval fails.
   */
  std::vector<amdsmi_processor_handle>
  get_processor_handles(amdsmi_socket_handle &socket_handle) {
    uint32_t count;
    check_status(
        m_driver_api->get_processor_handles(socket_handle, &count, nullptr),
        "Fail to get processor count for provided socket!");
    std::vector<amdsmi_processor_handle> processor_handles(count);
    check_status(m_driver_api->get_processor_handles(socket_handle, &count,
                                                     processor_handles.data()),
                 "Failed to get processor handles for provided socket!");

    return processor_handles;
  }

private:
  /** Shared pointer to the driver interface used for SMI operations. */
  std::shared_ptr<driver_t> m_driver_api;
  /** AMD SMI driver version information. */
  version m_version;
};

} // namespace smi
