# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(loops-all-mid-10k-sp)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

target_sources(app PRIVATE src/loops-all-mid-10k-sp.c
    src/loops.c
    src/100.c
    src/100k.c
    src/100kdot.c
    src/10k.c
    src/10kdot.c
    src/1k.c
    src/1kdot.c
    src/32.c

    src/mith/th_lib.c
    src/mith/mith_workload.c
    src/mith/mith_lib.c
    src/mith/th_bignum.c
    src/mith/th_rand.c

    src/mith/al/th_al.c
    src/mith/al/al_smp.c
    src/mith/al/al_single.c
)

target_compile_definitions(app PRIVATE
    USE_FP32=1
)

