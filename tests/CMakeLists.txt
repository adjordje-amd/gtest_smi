find_package(GTest)

set(smi_tests_source 
    ${CMAKE_CURRENT_SOURCE_DIR}/smi/service_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/smi/processor_tests.cpp

)

add_executable(${CMAKE_PROJECT_NAME}-tests ${smi_tests_source})

target_link_libraries(${CMAKE_PROJECT_NAME}-tests PRIVATE smi-library GTest::gtest GTest::gtest_main GTest::gmock fmt::fmt)

target_compile_options(${CMAKE_PROJECT_NAME}-tests PRIVATE -coverage)
target_link_options(${CMAKE_PROJECT_NAME}-tests PRIVATE -coverage)
