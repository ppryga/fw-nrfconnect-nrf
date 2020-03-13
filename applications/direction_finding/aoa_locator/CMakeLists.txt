# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("aoa_locator" VERSION 0.1)

set(NRF_SUPPORTED_BUILD_TYPES
	ZDebug
	ZRelease
  )

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ZDebug)
endif()

zephyr_compile_definitions(
	ARM_MATH_CM4
)

zephyr_include_directories(
	.
 )

target_sources(app PRIVATE src/main.c
	src/dfe_local_config.c
	src/if.c
	src/protocol.c
	src/ble.c
	src/float_ring_buffer.c
	src/average_results.c
)

#Include DF library
set(DF_LIB_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../df_aoa_library)
#target_link_directories(app PUBLIC ${DF_LIB_BASE_PATH}/lib)
target_link_libraries(app PRIVATE ${DF_LIB_BASE_PATH}/lib/libdf-fpv4-sp-d16-${ZEPHYR_TOOLCHAIN_VARIANT}.a)
target_include_directories(app PRIVATE ${DF_LIB_BASE_PATH}/include)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/bluetooth)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller)
