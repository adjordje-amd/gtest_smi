find_package(GTest)

add_executable(${CMAKE_PROJECT_NAME}-tests smi/service_tests.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}-tests PRIVATE smi-library GTest::gtest GTest::gtest_main GTest::gmock fmt::fmt)

target_compile_options(${CMAKE_PROJECT_NAME}-tests PRIVATE -coverage)
target_link_options(${CMAKE_PROJECT_NAME}-tests PRIVATE -coverage)
