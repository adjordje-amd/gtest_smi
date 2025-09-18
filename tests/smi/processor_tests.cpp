#include "smi/processor.hpp"
#include "gmock/gmock.h"
#include <amd_smi/amdsmi.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <sstream>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

// Mock driver API for processor testing
struct mock_processor_driver {
  MOCK_METHOD(amdsmi_status_t, get_power_info,
              (amdsmi_processor_handle, amdsmi_power_info_t *), ());
  MOCK_METHOD(amdsmi_status_t, get_gpu_activity,
              (amdsmi_processor_handle, amdsmi_engine_usage_t *), ());
  MOCK_METHOD(amdsmi_status_t, get_memory_usage,
              (amdsmi_processor_handle, amdsmi_memory_type_t, uint64_t *), ());
  MOCK_METHOD(amdsmi_status_t, get_temperature_metric,
              (amdsmi_processor_handle, amdsmi_temperature_type_t,
               amdsmi_temperature_metric_t, int64_t *),
              ());
  MOCK_METHOD(amdsmi_status_t, get_gpu_metrics_info,
              (amdsmi_processor_handle, amdsmi_gpu_metrics_t *), ());
  MOCK_METHOD(amdsmi_status_t, get_gpu_memory_usage,
              (amdsmi_processor_handle, amdsmi_memory_type_t, uint64_t *), ());
};

class ProcessorTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_driver = std::make_shared<StrictMock<mock_processor_driver>>();
    processor_handle = reinterpret_cast<amdsmi_processor_handle>(0x12345);
    processor_type = AMDSMI_PROCESSOR_TYPE_AMD_GPU;

    // Create processor instance
    test_processor =
        std::make_unique<rocprofsys::amd_smi::processor<mock_processor_driver>>(
            mock_driver, processor_handle, processor_type);
  }

  void TearDown() override {
    test_processor.reset();
    mock_driver.reset();
  }

  std::shared_ptr<StrictMock<mock_processor_driver>> mock_driver;
  amdsmi_processor_handle processor_handle;
  processor_type_t processor_type;
  std::unique_ptr<rocprofsys::amd_smi::processor<mock_processor_driver>>
      test_processor;
};

TEST_F(ProcessorTest, ConstructorInitializesCorrectly) {
  EXPECT_EQ(test_processor->get_processor_type(),
            AMDSMI_PROCESSOR_TYPE_AMD_GPU);
}

TEST_F(ProcessorTest, GetProcessorTypeReturnsCorrectType) {
  EXPECT_EQ(test_processor->get_processor_type(), processor_type);

  // Test with CPU type
  auto cpu_processor =
      std::make_unique<rocprofsys::amd_smi::processor<mock_processor_driver>>(
          mock_driver, processor_handle, AMDSMI_PROCESSOR_TYPE_AMD_CPU);
  EXPECT_EQ(cpu_processor->get_processor_type(), AMDSMI_PROCESSOR_TYPE_AMD_CPU);
}

TEST_F(ProcessorTest, GetSupportedMetricsAllSupported) {
  // Setup successful responses for all metrics
  amdsmi_power_info_t power_info = {};
  power_info.average_socket_power = 150;
  power_info.current_socket_power = 140;

  amdsmi_engine_usage_t engine_usage = {};

  uint64_t memory_usage = 8192;
  int64_t temperature = 123;

  amdsmi_gpu_metrics_t gpu_metrics = {};
  // Initialize VCN and JPEG activity arrays
  for (size_t i = 0; i < AMDSMI_MAX_NUM_VCN; ++i) {
    gpu_metrics.vcn_activity[i] = 50;
  }
  for (size_t i = 0; i < AMDSMI_MAX_NUM_JPEG_ENG_V1; ++i) {
    gpu_metrics.jpeg_activity[i] = 30;
  }

  // Initialize XCP stats
  for (size_t i = 0; i < AMDSMI_MAX_NUM_XCP; ++i) {
    for (size_t j = 0; j < AMDSMI_MAX_NUM_VCN; ++j) {
      gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
    }
    for (auto j{0}; j < AMDSMI_MAX_NUM_JPEG; ++j) {
      gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
    }
  }

  EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(power_info), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(engine_usage), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*mock_driver,
              get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
      .WillOnce(
          DoAll(SetArgPointee<2>(memory_usage), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*mock_driver,
              get_temperature_metric(processor_handle,
                                     AMDSMI_TEMPERATURE_TYPE_HOTSPOT,
                                     AMDSMI_TEMP_CURRENT, _))
      .WillOnce(
          DoAll(SetArgPointee<3>(temperature), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*mock_driver, get_temperature_metric(processor_handle,
                                                   AMDSMI_TEMPERATURE_TYPE_EDGE,
                                                   AMDSMI_TEMP_CURRENT, _))
      .WillOnce(
          DoAll(SetArgPointee<3>(temperature), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(gpu_metrics), Return(AMDSMI_STATUS_SUCCESS)));

  auto metrics = test_processor->get_supported_metrics();

  EXPECT_TRUE(metrics.average_socket_power);
  EXPECT_TRUE(metrics.current_socket_power);
  EXPECT_TRUE(metrics.gfx_activity);
  EXPECT_TRUE(metrics.mm_activity);
  EXPECT_TRUE(metrics.umc_activity);
  EXPECT_TRUE(metrics.memory_usage);
  EXPECT_TRUE(metrics.edge_temperature);
  EXPECT_TRUE(metrics.hotspot_temperature);
  EXPECT_TRUE(metrics.vcn_xcp_stats);
  EXPECT_TRUE(metrics.jpeg_xcp_stats);
}

// TEST_F(ProcessorTest, GetSupportedMetricsPowerInfoFails) {
//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;
//   amdsmi_gpu_metrics_t gpu_metrics = {};

//   // Initialize XCP stats with supported values
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
//     }
//   }

//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .WillOnce(Return(AMDSMI_STATUS_NOT_SUPPORTED));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   auto metrics = test_processor->get_supported_metrics();

//   EXPECT_FALSE(metrics.average_socket_power);
//   EXPECT_FALSE(metrics.current_socket_power);
//   EXPECT_TRUE(metrics.gfx_activity);
//   EXPECT_TRUE(metrics.memory_usage);
//   EXPECT_TRUE(metrics.edge_temperature);
//   EXPECT_TRUE(metrics.hotspot_temperature);
// }

// TEST_F(ProcessorTest, GetSmiMetricsSuccess) {
//   // First call for get_supported_metrics
//   amdsmi_power_info_t power_info = {};
//   power_info.average_socket_power = 150;
//   power_info.current_socket_power = 140;

//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;

//   amdsmi_gpu_metrics_t gpu_metrics = {};
//   gpu_metrics.average_socket_power = 150;
//   gpu_metrics.current_socket_power = 140;
//   gpu_metrics.average_gfx_activity = 75;
//   gpu_metrics.average_umc_activity = 50;
//   gpu_metrics.average_mm_activity = 60;
//   gpu_metrics.temperature_hotspot = 65000;

//   // Initialize XCP stats with supported values
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
//     }
//   }

//   // Calls for get_supported_metrics
//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(power_info),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   // Calls for get_smi_metrics
//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_gpu_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM,
//               _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   auto metrics = test_processor->get_smi_metrics();

//   EXPECT_EQ(metrics.average_socket_power, 150);
//   EXPECT_EQ(metrics.current_socket_power, 140);
//   EXPECT_EQ(metrics.memory_usage, 8192);
//   EXPECT_EQ(metrics.gfx_activity, 75);
//   EXPECT_EQ(metrics.umc_activity, 50);
//   EXPECT_EQ(metrics.mm_activity, 60);
//   EXPECT_EQ(metrics.hotspot_temperature, 65000);
//   EXPECT_EQ(metrics.edge_temperature, 65000);
// }

// TEST_F(ProcessorTest, GetSmiMetricsGpuMetricsFails) {
//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(Return(AMDSMI_STATUS_NOT_SUPPORTED));

//   EXPECT_THROW(test_processor->get_smi_metrics(), std::runtime_error);
// }

// TEST_F(ProcessorTest, GetSmiMetricsMemoryUsageFails) {
//   amdsmi_gpu_metrics_t gpu_metrics = {};

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_gpu_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM,
//               _))
//       .WillOnce(Return(AMDSMI_STATUS_NOT_SUPPORTED));

//   // Should not throw, just log error and continue
//   EXPECT_NO_THROW(test_processor->get_smi_metrics());
// }

// TEST_F(ProcessorTest, PrintSupportedMetricsOutput) {
//   // Setup minimal supported metrics
//   amdsmi_power_info_t power_info = {};
//   power_info.average_socket_power = 150;
//   power_info.current_socket_power = 140;

//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;
//   amdsmi_gpu_metrics_t gpu_metrics = {};

//   // Initialize XCP stats
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
//     }
//   }

//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(power_info),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   // Capture stdout
//   std::stringstream buffer;
//   std::streambuf *orig = std::cout.rdbuf(buffer.rdbuf());

//   test_processor->print_supported_metrics();

//   // Restore stdout
//   std::cout.rdbuf(orig);

//   std::string output = buffer.str();
//   EXPECT_THAT(output, ::testing::HasSubstr("=== SUPPORTED SMI METRICS ==="));
//   EXPECT_THAT(output, ::testing::HasSubstr("current_socket_power"));
//   EXPECT_THAT(output, ::testing::HasSubstr("average_socket_power"));
//   EXPECT_THAT(output, ::testing::HasSubstr("memory_usage"));
//   EXPECT_THAT(output, ::testing::HasSubstr("temperature"));
// }

// TEST_F(ProcessorTest, BitsetToIndexListFunction) {
//   std::bitset<8> test_bitset;
//   test_bitset[0] = true;
//   test_bitset[2] = true;
//   test_bitset[7] = true;

//   auto result = rocprofsys::amd_smi::bitset_to_index_list(test_bitset);
//   EXPECT_EQ(result, "[true, false, true, false, false, false, false, true]");
// }

// TEST_F(ProcessorTest, BitsetToIndexListEmpty) {
//   std::bitset<8> empty_bitset;

//   auto result = rocprofsys::amd_smi::bitset_to_index_list(empty_bitset);
//   EXPECT_EQ(result, "[false, false, false, false, false, false, false,
//   false]");
// }

// TEST_F(ProcessorTest, MetricValueNotSupportedConstant) {
//   constexpr auto expected_value = 0xffff;
//   EXPECT_EQ(rocprofsys::amd_smi::metric_value_not_supported, expected_value);
// }

// TEST_F(ProcessorTest, GetSupportedMetricsXcpStatsUnsupported) {
//   amdsmi_power_info_t power_info = {};
//   power_info.average_socket_power = 150;
//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;

//   amdsmi_gpu_metrics_t gpu_metrics = {};
//   // Set XCP stats to unsupported values
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] =
//           rocprofsys::amd_smi::metric_value_not_supported;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] =
//           rocprofsys::amd_smi::metric_value_not_supported;
//     }
//   }

//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(power_info),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   auto metrics = test_processor->get_supported_metrics();

//   EXPECT_FALSE(metrics.vcn_xcp_stats);
//   EXPECT_FALSE(metrics.jpeg_xcp_stats);
// }

// TEST_F(ProcessorTest, GetSupportedMetricsVcnAndJpegActivityEngines) {
//   amdsmi_power_info_t power_info = {};
//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;

//   amdsmi_gpu_metrics_t gpu_metrics = {};
//   // Set some VCN and JPEG engines to supported, others to unsupported
//   gpu_metrics.vcn_activity[0] = 50;
//   gpu_metrics.vcn_activity[1] =
//   rocprofsys::amd_smi::metric_value_not_supported;
//   gpu_metrics.vcn_activity[2] = 30;

//   gpu_metrics.jpeg_activity[0] = 40;
//   gpu_metrics.jpeg_activity[1] =
//       rocprofsys::amd_smi::metric_value_not_supported;

//   // Initialize XCP stats
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
//     }
//   }

//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .WillOnce(Return(AMDSMI_STATUS_NOT_SUPPORTED));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   auto metrics = test_processor->get_supported_metrics();

//   // Check VCN activity engines
//   EXPECT_TRUE(metrics.vcn_activity_engine[0]);  // Supported
//   EXPECT_FALSE(metrics.vcn_activity_engine[1]); // Unsupported
//   if (AMDSMI_MAX_NUM_VCN > 2) {
//     EXPECT_TRUE(metrics.vcn_activity_engine[2]); // Supported
//   }

//   // Check JPEG activity engines
//   EXPECT_TRUE(metrics.jpeg_activity_engine[0]); // Supported
//   if (AMDSMI_MAX_NUM_JPEG_ENG_V1 > 1) {
//     EXPECT_FALSE(metrics.jpeg_activity_engine[1]); // Unsupported
//   }
// }

// TEST_F(ProcessorTest, GetSupportedMetricsStaticBehavior) {
//   // This test verifies that get_supported_metrics uses a static lambda
//   // and should return the same results on subsequent calls

//   amdsmi_power_info_t power_info = {};
//   power_info.average_socket_power = 150;

//   amdsmi_engine_usage_t engine_usage = {};
//   uint64_t memory_usage = 8192;
//   int64_t temperature = 65000;
//   amdsmi_gpu_metrics_t gpu_metrics = {};

//   // Initialize XCP stats
//   for (size_t i = 0; i < 8; ++i) {
//     for (size_t j = 0; j < 8; ++j) {
//       gpu_metrics.xcp_stats[i].vcn_busy[j] = 25;
//       gpu_metrics.xcp_stats[i].jpeg_busy[j] = 20;
//     }
//   }

//   // Expect calls only once due to static lambda
//   EXPECT_CALL(*mock_driver, get_power_info(processor_handle, _))
//       .Times(1)
//       .WillOnce(
//           DoAll(SetArgPointee<1>(power_info),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_activity(processor_handle, _))
//       .Times(1)
//       .WillOnce(
//           DoAll(SetArgPointee<1>(engine_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_memory_usage(processor_handle, AMDSMI_MEM_TYPE_VRAM, _))
//       .Times(1)
//       .WillOnce(
//           DoAll(SetArgPointee<2>(memory_usage),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver,
//               get_temperature_metric(processor_handle,
//                                      AMDSMI_TEMPERATURE_TYPE_JUNCTION,
//                                      AMDSMI_TEMP_CURRENT, _))
//       .Times(1)
//       .WillOnce(
//           DoAll(SetArgPointee<3>(temperature),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   EXPECT_CALL(*mock_driver, get_gpu_metrics_info(processor_handle, _))
//       .Times(1)
//       .WillOnce(
//           DoAll(SetArgPointee<1>(gpu_metrics),
//           Return(AMDSMI_STATUS_SUCCESS)));

//   // Call multiple times
//   auto metrics1 = test_processor->get_supported_metrics();
//   auto metrics2 = test_processor->get_supported_metrics();

//   // Should be identical
//   EXPECT_EQ(metrics1.average_socket_power, metrics2.average_socket_power);
//   EXPECT_EQ(metrics1.memory_usage, metrics2.memory_usage);
// }
