add_library(smi-library INTERFACE)

find_package(amd-smi REQUIRED)

target_include_directories(smi-library INTERFACE ${CMAKE_CURRENT_LIST_DIR}/)
target_link_libraries(smi-library INTERFACE amd-smi::amd-smi)
