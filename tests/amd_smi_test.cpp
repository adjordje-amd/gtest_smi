#include "gmock/gmock.h"
#include <amd_smi/amdsmi.h>
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

#include "amd_smi.hpp"

struct mock_amd_smi_api {
  MOCK_METHOD(amdsmi_status_t, init, (uint64_t));
  MOCK_METHOD(amdsmi_status_t, get_socket_handles,
              (uint32_t *, amdsmi_socket_handle *));
  MOCK_METHOD(amdsmi_status_t, get_processor_handles,
              (amdsmi_socket_handle, uint32_t *, amdsmi_processor_handle *));
  MOCK_METHOD(amdsmi_status_t, get_power_info,
              (amdsmi_processor_handle, amdsmi_power_info_t *));
  MOCK_METHOD(amdsmi_status_t, get_temperature_metric,
              (amdsmi_processor_handle, amdsmi_temperature_type_t,
               amdsmi_temperature_metric_t, int64_t *));
  MOCK_METHOD(amdsmi_status_t, get_processor_type,
              (amdsmi_processor_handle, processor_type_t *));
};

class amd_smi_test : public ::testing::Test {
protected:
  void SetUp() override {
    instance = std::make_unique<amd_smi<mock_amd_smi_api>>(&api);
    // Calling api with nullptr for the second argument, we are just fetching
    // the socket count
    ON_CALL(api, get_socket_handles(testing::_, nullptr))
        .WillByDefault(testing::DoAll(testing::SetArgPointee<0>(1),
                                      testing::Return(AMDSMI_STATUS_SUCCESS)));

    // Second call is to collect socket handlers
    ON_CALL(api, get_socket_handles(testing::_, testing::NotNull()))
        .WillByDefault(testing::DoAll(
            testing::SetArgPointee<0>(1), // Set socket_count to 1
            testing::Return(AMDSMI_STATUS_SUCCESS)));

    ON_CALL(api, get_processor_handles(testing::_, testing::_, testing::_))
        .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(1),
                                      testing::Return(AMDSMI_STATUS_SUCCESS)));
  }

  void TearDown() override { instance.reset(); }

  mock_amd_smi_api api;
  std::unique_ptr<amd_smi<mock_amd_smi_api>> instance;
};

TEST_F(amd_smi_test, init_driver) {
  EXPECT_CALL(api, init(testing::_))
      .Times(1)
      .WillRepeatedly(testing::Return(AMDSMI_STATUS_SUCCESS));
  ASSERT_NO_THROW(instance->init_driver());
}

TEST_F(amd_smi_test, init_driver_error) {
  EXPECT_CALL(api, init(testing::_))
      .Times(1)
      .WillRepeatedly(testing::Return(AMDSMI_STATUS_INIT_ERROR));
  ASSERT_THROW(instance->init_driver(), std::runtime_error);
}

TEST_F(amd_smi_test, get_processor_handles_empty) {
  EXPECT_CALL(api, get_processor_handles(testing::_, testing::_, testing::_)).Times(2).WillRepeatedly(testing::DoAll(testing::SetArgPointee<1>(0), testing::Return(AMDSMI_STATUS_SUCCESS)));
  ASSERT_NO_THROW(instance->read_sample());
}

TEST_F(amd_smi_test, get_socket_handles_success) {
  EXPECT_CALL(api, get_socket_handles(testing::_, nullptr));
  EXPECT_CALL(api, get_socket_handles(testing::_, testing::NotNull()));

  auto result = instance->get_socket_handles();
  EXPECT_EQ(result.size(), 1);
}

TEST_F(amd_smi_test, get_socket_handles_first_call_failure) {
  EXPECT_CALL(api, get_socket_handles(testing::_, nullptr))
      .Times(1)
      .WillOnce(testing::Return(AMDSMI_STATUS_INIT_ERROR));

  auto result = instance->get_socket_handles();
  EXPECT_TRUE(result.empty());
}

TEST_F(amd_smi_test, get_socket_handles_second_call_failure) {
  EXPECT_CALL(api, get_socket_handles(testing::_, nullptr));
  EXPECT_CALL(api, get_socket_handles(testing::_, testing::NotNull()))
      .Times(1)
      .WillOnce(testing::Return(AMDSMI_STATUS_INIT_ERROR));

  auto result = instance->get_socket_handles();
  EXPECT_TRUE(result.empty());
}

TEST_F(amd_smi_test, get_socket_handles_zero_sockets) {
  EXPECT_CALL(api, get_socket_handles(testing::_, nullptr))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgPointee<0>(0),
                               testing::Return(AMDSMI_STATUS_SUCCESS)));

  auto result = instance->get_socket_handles();
  EXPECT_TRUE(result.empty());
}

TEST_F(amd_smi_test, read_sample) {
  EXPECT_CALL(api, get_socket_handles).Times(2);
  EXPECT_CALL(api, get_processor_handles).Times(2);
  EXPECT_CALL(api, get_processor_type(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(
          testing::SetArgPointee<1>(AMDSMI_PROCESSOR_TYPE_AMD_GPU),
          testing::Return(AMDSMI_STATUS_SUCCESS)));

  amdsmi_power_info_t power_info{
      .socket_power = 11,
      .current_socket_power = 10,
      .average_socket_power = 9,
  };

  EXPECT_CALL(api, get_power_info(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgPointee<1>(power_info),
                               testing::Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(api, get_temperature_metric(testing::_, testing::_, testing::_,
                                          testing::_))
      .Times(1)
      .WillOnce(testing::DoAll(testing::SetArgPointee<3>(23),
                               testing::Return(AMDSMI_STATUS_SUCCESS)));

  instance->read_sample();
}
