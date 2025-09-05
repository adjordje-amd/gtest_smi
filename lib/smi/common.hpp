#pragma once

#include <amd_smi/amdsmi.h>
#include <fmt/format.h>

namespace smi {

struct version {
  struct {
    uint32_t major;
    uint32_t minor;
    uint32_t release;
  } numeric_representation;
  std::string string_representation;
};

inline void check_status(const amdsmi_status_t &status,
                         const char *error_message) {
  if (status != AMDSMI_STATUS_SUCCESS) {
    throw std::runtime_error(
        fmt::format("{} Error: {}", error_message, status));
  }
};

} // namespace smi
