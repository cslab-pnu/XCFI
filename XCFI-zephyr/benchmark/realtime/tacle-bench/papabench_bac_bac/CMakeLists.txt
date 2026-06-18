cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(papabench_zephyr)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/include
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/include/c
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/var/include
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/airborne/autopilot
  ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/airborne/fly_by_wire/
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(app PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/var/include/airframe.h
  -include ${CMAKE_CURRENT_SOURCE_DIR}/papabench/sw/var/include/radio.h
)

target_sources(app PRIVATE

  src/main.c
  src/pb_port.c
  src/pb_sched.c

  papabench/sw/airborne/autopilot/mainloop.c
  papabench/sw/airborne/autopilot/main.c
  papabench/sw/airborne/autopilot/estimator.c
  papabench/sw/airborne/autopilot/nav.c
  papabench/sw/airborne/autopilot/pid.c
  papabench/sw/airborne/autopilot/if_calib.c

  papabench/sw/lib/c/math.c
  papabench/sw/lib/c/geometry.c
)

