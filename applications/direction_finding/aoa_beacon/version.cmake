#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
##############################################################################

# version.cmake
# -------------
#
# Inputs:
#
#   ``*VERSION*`` and other constants set in VERSION file
#
# Outputs with examples::
#
#   PROJECT_VERSION         1.14.99.07
#   APP_VERSION_STRING      "1.14.99-extraver"
#
#   APP_VERSION_MAJOR       1
#   APP_VERSION_MINOR       14
#   APP_PATCHLEVEL          99
#
# Most outputs are converted to C macros, see ``app_version.h.in``
#

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION ver)

string(REGEX MATCH "VERSION_MAJOR = ([0-9]*)" _ ${ver})
set(PROJECT_APP_VERSION_MAJOR ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR = ([0-9]*)" _ ${ver})
set(PROJECT_APP_VERSION_MINOR ${CMAKE_MATCH_1})

string(REGEX MATCH "PATCHLEVEL = ([0-9]*)" _ ${ver})
set(PROJECT_APP_VERSION_PATCH ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_TWEAK = ([0-9]*)" _ ${ver})
set(PROJECT_APP_VERSION_TWEAK ${CMAKE_MATCH_1})

string(REGEX MATCH "EXTRAVERSION = ([a-z0-9]*)" _ ${ver})
set(PROJECT_APP_VERSION_EXTRA ${CMAKE_MATCH_1})

# Temporary convenience variable
set(PROJECT_APP_VERSION_WITHOUT_TWEAK ${PROJECT_APP_VERSION_MAJOR}.${PROJECT_APP_VERSION_MINOR}.${PROJECT_APP_VERSION_PATCH})

if(PROJECT_APP_VERSION_EXTRA)
  set(PROJECT_APP_VERSION_EXTRA_STR "-${PROJECT_VERSION_EXTRA}")
endif()

if(PROJECT_APP_VERSION_TWEAK)
  set(PROJECT_APP_VERSION ${PROJECT_APP_VERSION_WITHOUT_TWEAK}.${PROJECT_APP_VERSION_TWEAK})
else()
  set(PROJECT_APP_VERSION ${PROJECT_APP_VERSION_WITHOUT_TWEAK})
endif()

set(PROJECT_APP_VERSION_STR ${PROJECT_APP_VERSION}${PROJECT_APP_VERSION_EXTRA_STR})

if (NOT NO_PRINT_VERSION)
  message(STATUS "Application version: ${PROJECT_APP_VERSION_STR}")
endif()

set(APP_VERSION_MAJOR      ${PROJECT_APP_VERSION_MAJOR})
set(APP_VERSION_MINOR      ${PROJECT_APP_VERSION_MINOR})
set(APP_PATCHLEVEL         ${PROJECT_APP_VERSION_PATCH})

if(PROJECT_APP_VERSION_EXTRA)
  set(APP_VERSION_STRING     "\"${PROJECT_NAME}-${PROJECT_APP_VERSION_WITHOUT_TWEAK}-${PROJECT_APP_VERSION_EXTRA}\"")
else()
  set(APP_VERSION_STRING     "\"${PROJECT_NAME}-${PROJECT_APP_VERSION_WITHOUT_TWEAK}\"")
endif()

# Cleanup convenience variables
unset(PROJECT_APP_VERSION_WITHOUT_TWEAK)
