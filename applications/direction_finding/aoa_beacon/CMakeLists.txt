# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb)
set(ZEPHYR_BASE ../../zephyr)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("direction_fingin_beacon"
        VERSION 0.1)

set(NRF_SUPPORTED_BUILD_TYPES
  ZDebug
  ZRelease
  )

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

target_sources(app PRIVATE src/main.c)
