// tests/test_service_tests.cpp
#include "smi/service.hpp"
#include "gmock/gmock.h"
#include <amd_smi/amdsmi.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

// Mock driver API
struct mock_driver_api {
  MOCK_METHOD(amdsmi_status_t, init, (), ());
  MOCK_METHOD(amdsmi_status_t, get_version, (amdsmi_version_t *), ());
  MOCK_METHOD(amdsmi_status_t, get_socket_handles,
              (uint32_t *, amdsmi_socket_handle *), ());
  MOCK_METHOD(amdsmi_status_t, get_processor_handles,
              (amdsmi_socket_handle, uint32_t *, amdsmi_processor_handle *),
              ());
  MOCK_METHOD(amdsmi_status_t, get_processor_type,
              (amdsmi_processor_handle, processor_type_t *), ());
};

std::shared_ptr<mock_driver_api> g_mock_api_instance = nullptr;
// Mock driver factory
struct mock_driver_factory {
  using driver_t = mock_driver_api;
  static std::shared_ptr<mock_driver_api> create_driver() {
    return g_mock_api_instance;
  }
};

class ServiceTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_mock_api_instance = std::make_shared<mock_driver_api>();

    ON_CALL(*g_mock_api_instance, init())
        .WillByDefault(Return(AMDSMI_STATUS_SUCCESS));
    ON_CALL(*g_mock_api_instance, get_version(_))
        .WillByDefault([](amdsmi_version_t *v) {
          if (v)
            *v = amdsmi_version_t{1, 2, 3, "build123"};
          return AMDSMI_STATUS_SUCCESS;
        });
    ON_CALL(*g_mock_api_instance, get_socket_handles(testing::_, nullptr))
        .WillByDefault(
            DoAll(SetArgPointee<0>(1), testing::Return(AMDSMI_STATUS_SUCCESS)));

    // Second call is to collect socket handlers
    ON_CALL(*g_mock_api_instance,
            get_socket_handles(testing::_, testing::NotNull()))
        .WillByDefault(DoAll(SetArgPointee<0>(1), // Set socket_count to 1
                             testing::Return(AMDSMI_STATUS_SUCCESS)));

    ON_CALL(*g_mock_api_instance, get_processor_handles(_, _, nullptr))
        .WillByDefault(
            DoAll(SetArgPointee<1>(1), Return(AMDSMI_STATUS_SUCCESS)));

    ON_CALL(*g_mock_api_instance,
            get_processor_handles(_, _, testing::NotNull()))
        .WillByDefault(
            DoAll(SetArgPointee<1>(1), Return(AMDSMI_STATUS_SUCCESS)));
  }
  void TearDown() override { g_mock_api_instance.reset(); }
};

TEST_F(ServiceTest, ConstructSuccess) {
  amdsmi_version_t version{1, 2, 3, "build123"};
  EXPECT_CALL(*g_mock_api_instance, init);
  EXPECT_CALL(*g_mock_api_instance, get_version(_));

  EXPECT_NO_THROW({ smi::service<mock_driver_factory> svc; });
}

TEST_F(ServiceTest, ConstructInitFail) {
  EXPECT_CALL(*g_mock_api_instance, init())
      .WillOnce(Return(AMDSMI_STATUS_INIT_ERROR));
  EXPECT_THROW({ smi::service<mock_driver_factory> svc; }, std::runtime_error);
}

TEST_F(ServiceTest, ConstructVersionFail) {
  EXPECT_CALL(*g_mock_api_instance, init);
  EXPECT_CALL(*g_mock_api_instance, get_version(_))
      .WillOnce(Return(AMDSMI_STATUS_INIT_ERROR));
  EXPECT_THROW({ smi::service<mock_driver_factory> svc; }, std::runtime_error);
}

TEST_F(ServiceTest, GetVersionReturnsCorrect) {
  amdsmi_version_t version{4, 5, 6, "build456"};
  EXPECT_CALL(*g_mock_api_instance, init);
  EXPECT_CALL(*g_mock_api_instance, get_version(_))
      .WillOnce(
          DoAll(SetArgPointee<0>(version), Return(AMDSMI_STATUS_SUCCESS)));
  smi::service<mock_driver_factory> svc;
  auto v = svc.get_version();
  EXPECT_EQ(v.numeric_representation.major, 4);
  EXPECT_EQ(v.numeric_representation.minor, 5);
  EXPECT_EQ(v.numeric_representation.release, 6);
  EXPECT_EQ(v.string_representation, "build456");
}

TEST_F(ServiceTest, GetProcessorsSuccess) {
  // Setup: 1 socket, 2 processors
  EXPECT_CALL(*g_mock_api_instance, init());
  EXPECT_CALL(*g_mock_api_instance, get_version(_));
  EXPECT_CALL(*g_mock_api_instance, get_socket_handles(_, nullptr))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(AMDSMI_STATUS_SUCCESS)));
  EXPECT_CALL(*g_mock_api_instance, get_socket_handles(_, testing::NotNull()))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<0>(1), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*g_mock_api_instance, get_processor_handles(_, _, nullptr))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(2), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*g_mock_api_instance,
              get_processor_handles(_, _, testing::NotNull()))
      .Times(1)
      .WillOnce(DoAll(SetArgPointee<1>(2), Return(AMDSMI_STATUS_SUCCESS)));

  EXPECT_CALL(*g_mock_api_instance, get_processor_type(_, _))
      .Times(2)
      .WillRepeatedly(DoAll(SetArgPointee<1>(AMDSMI_PROCESSOR_TYPE_AMD_CPU),
                            Return(AMDSMI_STATUS_SUCCESS)));

  smi::service<mock_driver_factory> svc;
  auto processors = svc.get_processors();
  EXPECT_EQ(processors.size(), 2);
}

TEST_F(ServiceTest, GetProcessorsSocketFail) {
  EXPECT_CALL(*g_mock_api_instance, init());
  EXPECT_CALL(*g_mock_api_instance, get_version(_));
  EXPECT_CALL(*g_mock_api_instance, get_socket_handles(_, nullptr))
      .WillOnce(Return(AMDSMI_STATUS_INIT_ERROR));
  smi::service<mock_driver_factory> svc;
  EXPECT_THROW(svc.get_processors(), std::runtime_error);
}

TEST_F(ServiceTest, GetProcessorsProcessorHandlesFail) {
  EXPECT_CALL(*g_mock_api_instance, init());
  EXPECT_CALL(*g_mock_api_instance, get_version(_));
  EXPECT_CALL(*g_mock_api_instance, get_socket_handles).Times(2);
  EXPECT_CALL(*g_mock_api_instance, get_processor_handles(_, _, nullptr))
      .WillOnce(Return(AMDSMI_STATUS_INIT_ERROR));
  smi::service<mock_driver_factory> svc;
  EXPECT_THROW(svc.get_processors(), std::runtime_error);
}

TEST_F(ServiceTest, GetProcessorsProcessorTypeFail) {
  EXPECT_CALL(*g_mock_api_instance, init());
  EXPECT_CALL(*g_mock_api_instance, get_version(_));
  EXPECT_CALL(*g_mock_api_instance, get_socket_handles(_, _)).Times(2);
  EXPECT_CALL(*g_mock_api_instance, get_processor_handles(_, _, _)).Times(2);
  EXPECT_CALL(*g_mock_api_instance, get_processor_type(_, _))
      .WillOnce(Return(AMDSMI_STATUS_INIT_ERROR));
  smi::service<mock_driver_factory> svc;
  EXPECT_THROW(svc.get_processors(), std::runtime_error);
}
