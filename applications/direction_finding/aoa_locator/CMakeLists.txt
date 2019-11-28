# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
set(ZEPHYR_BASE ../../zephyr)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("aoa_locator"
        VERSION 0.1)

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
#    src/math
 )

#FILE(GLOB_RECURSE app_sources src/*.c)
#target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE src/main.c
	src/if.c
	src/protocol.c
	src/ble.c
	src/common.c
)

set(DF_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../df_aoa_library/lib)
zephyr_library_link_libraries(${DF_LIB_PATH}/libdf-fpv4-sp-d16.a)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../df_aoa_library/include)
zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
zephyr_library_include_directories($ENV{ZEPHYR_BASE}/subsys/bluetooth/controller)
