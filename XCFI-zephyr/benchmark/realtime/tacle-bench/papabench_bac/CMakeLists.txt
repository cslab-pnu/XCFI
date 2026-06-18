# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_compile_options(app PRIVATE "-include${CMAKE_CURRENT_SOURCE_DIR}/src/var_include/fake_avr.h")

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/var_include
  ${CMAKE_CURRENT_SOURCE_DIR}/src/var_include/arch
)

target_sources(app PRIVATE src/main.c
  src/fbw/fbw.c
  src/autopilot/autopilot.c
  src/autopilot/autopilotloop.c
)
