# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_ret2ns)

target_sources(app PRIVATE
  src/main.c
)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DCONFIG_TFM_BUILD_LOG_QUIET=OFF
  -DTFM_EXCEPTION_INFO_DUMP=ON
  -DTFM_LOG_NONFATAL_ERRORS=ON
  -DTFM_LOG_FATAL_ERRORS=ON
  -DTFM_SPM_LOG_LEVEL=TFM_SPM_LOG_LEVEL_DEBUG
  -DTFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_DEBUG
  -DCOMPILE_MODE=${COMPILE_MODE}
)
