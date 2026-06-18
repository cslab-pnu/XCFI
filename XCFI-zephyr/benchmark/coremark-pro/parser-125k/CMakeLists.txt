# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(parser-125k)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

target_sources(app PRIVATE src/parser-125k.c
    src/parser-125k.c
    src/ezxml.c
    src/parser.c
    src/stubs.c

    src/mith/th_lib.c
    src/mith/mith_workload.c
    src/mith/mith_lib.c
    src/mith/th_bignum.c
    src/mith/th_rand.c

    src/mith/al/th_al.c
    src/mith/al/al_smp.c
    src/mith/al/al_single.c
    src/mith/al/al_file.c
)

target_compile_definitions(app PRIVATE
  EZXML_NOMMAP=1
)

