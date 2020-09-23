# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("aoa_locator_tests" VERSION 0.1)

set(NRF_SUPPORTED_BUILD_TYPES
	ZDebug
	ZRelease
  )

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ZDebug)
endif()

#Include DF library
set(DF_LIB_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../df_aoa_library)
target_include_directories(app PRIVATE ${DF_LIB_BASE_PATH}/include
					../src)

#application settins
target_sources(app PRIVATE src/main.c
	src/dfe_samples_mapping_test.c
	src/configuration_fixtures.c
	src/dfe_configuration_tests.c
	../src/dfe_data_preprocess.c
	../src/dfe_local_config.c)

target_sources(app PRIVATE ${app_sources})
