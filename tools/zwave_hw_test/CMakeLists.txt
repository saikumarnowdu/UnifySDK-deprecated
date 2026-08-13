cmake_minimum_required(VERSION 3.21)
project(zwave_hw_test LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor")
set(ZWAPI_DIR "${VENDOR_DIR}/zwave_api")
set(ZWAVE_DEF_DIR "${VENDOR_DIR}/zwave_definitions")
set(STUB_DIR "${CMAKE_CURRENT_LIST_DIR}/stubs")

if(NOT EXISTS "${ZWAPI_DIR}/src/zwapi_init.c")
  message(FATAL_ERROR
    "zwapi vendor missing. Run: tools/zwave_hw_test/scripts/extract_zwapi_vendor.sh")
endif()

set(ZWAPI_PLATFORM posix)

add_library(zwave_hw_zwapi STATIC
  ${ZWAPI_DIR}/src/zwapi_connection.c
  ${ZWAPI_DIR}/src/zwapi_init.c
  ${ZWAPI_DIR}/src/zwapi_protocol_basis.c
  ${ZWAPI_DIR}/src/zwapi_protocol_controller.c
  ${ZWAPI_DIR}/src/zwapi_protocol_mem.c
  ${ZWAPI_DIR}/src/zwapi_protocol_nvm.c
  ${ZWAPI_DIR}/src/zwapi_protocol_rx_dispatch.c
  ${ZWAPI_DIR}/src/zwapi_protocol_transport.c
  ${ZWAPI_DIR}/src/zwapi_session.c
  ${ZWAPI_DIR}/src/zwapi_utils.c
  ${ZWAPI_DIR}/platform/${ZWAPI_PLATFORM}/zwapi_serial.c
  ${ZWAPI_DIR}/platform/${ZWAPI_PLATFORM}/zwapi_timestamp.c
  ${STUB_DIR}/sl_log_stub.c
)

target_include_directories(zwave_hw_zwapi
  PUBLIC
    ${ZWAPI_DIR}/include
    ${ZWAVE_DEF_DIR}/include
    ${VENDOR_DIR}
    ${REPO_ROOT}/include
  PRIVATE
    ${ZWAPI_DIR}/src
    ${ZWAPI_DIR}/platform/${ZWAPI_PLATFORM}
    ${STUB_DIR}
)

set_property(TARGET zwave_hw_zwapi PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(zwave_python_harness SHARED
  ${CMAKE_CURRENT_LIST_DIR}/harness/zwave_python_harness.c
)

target_link_libraries(zwave_python_harness PRIVATE zwave_hw_zwapi)
target_include_directories(zwave_python_harness
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/harness
  PRIVATE
    ${REPO_ROOT}/include
    ${STUB_DIR}
    ${ZWAPI_DIR}/include
    ${ZWAPI_DIR}/src
)

set_target_properties(zwave_python_harness PROPERTIES
  OUTPUT_NAME "zwave_python_harness"
  POSITION_INDEPENDENT_CODE ON
)

install(TARGETS zwave_python_harness LIBRARY DESTINATION lib)
