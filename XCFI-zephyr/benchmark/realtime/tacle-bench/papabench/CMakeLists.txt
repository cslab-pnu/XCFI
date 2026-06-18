cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(papabench_zephyr)

target_compile_definitions(app PRIVATE
  PAPABENCH_SINGLE=1
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/var/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/airborne/autopilot/
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/airborne/fly_by_wire/
)

target_compile_options(app PRIVATE
)

target_sources(app PRIVATE
  src/fake_hw.c

  papabench/sw/lib/c/math.c

  papabench/sw/airborne/autopilot/mainloop.c
  papabench/sw/airborne/autopilot/main.c
  papabench/sw/airborne/autopilot/gps_ubx.c
  papabench/sw/airborne/autopilot/modem.c
  papabench/sw/airborne/autopilot/spi.c
  papabench/sw/airborne/autopilot/nav.c
  papabench/sw/airborne/autopilot/infrared.c
  papabench/sw/airborne/autopilot/estimator.c
  papabench/sw/airborne/autopilot/if_calib.c
  papabench/sw/airborne/autopilot/pid.c
  papabench/sw/airborne/autopilot/link_fbw.c
  papabench/sw/airborne/autopilot/adc.c
  papabench/sw/airborne/autopilot/uart.c

  papabench/sw/airborne/fly_by_wire/main.c
  papabench/sw/airborne/fly_by_wire/ppm.c
  papabench/sw/airborne/fly_by_wire/servo.c
  papabench/sw/airborne/fly_by_wire/spi.c
  papabench/sw/airborne/fly_by_wire/adc_fbw.c
  papabench/sw/airborne/fly_by_wire/uart.c
)

set_source_files_properties(
  papabench/sw/airborne/autopilot/link_fbw.c
  papabench/sw/airborne/autopilot/spi.c
  papabench/sw/airborne/autopilot/adc.c
  PROPERTIES COMPILE_DEFINITIONS "SIGNAL_PREFIX=ap_"
)

set_source_files_properties(
  papabench/sw/airborne/fly_by_wire/ppm.c
  papabench/sw/airborne/fly_by_wire/servo.c
  papabench/sw/airborne/fly_by_wire/spi.c
  papabench/sw/airborne/fly_by_wire/adc_fbw.c
  PROPERTIES COMPILE_DEFINITIONS "SIGNAL_PREFIX=fbw_"
)
