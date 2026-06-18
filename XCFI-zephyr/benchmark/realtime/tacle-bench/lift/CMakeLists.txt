cmake_minimum_required(VERSION 3.20)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lift_zephyr)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
)

target_compile_options(app PRIVATE
)

target_sources(app PRIVATE
  src/main.c
  src/liftlibcontrol.c
  src/liftlibio.c
)
