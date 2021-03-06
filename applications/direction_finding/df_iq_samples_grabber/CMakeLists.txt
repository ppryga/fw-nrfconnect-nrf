#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("df_iq_samples_grabber")

include(${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)

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
	src/if.c
	src/protocol.c
	src/ble.c
  src/dfe_local_config.c
  src/app_version.c
)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/bluetooth)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller)
target_include_directories(app PRIVATE ${PROJECT_BINARY_DIR}/include)

configure_file("${PROJECT_SOURCE_DIR}/app_version.h.in" "${PROJECT_BINARY_DIR}/include/app_version.h")
include_directories()

zephyr_linker_sources(ROM_START app_version_sections.ld)
