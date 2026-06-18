# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cjpeg-rose7-preset)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

target_sources(app PRIVATE src/cjpeg-rose7-preset.c
    src/bm_lib.c
    src/bmark_lite.c
    src/cdjpeg.c
    src/cjpeg.c
    src/filedata.c
    src/jcapimin.c
    src/jcapistd.c
    src/jccoefct.c
    src/jccolor.c
    src/jcdctmgr.c
    src/jchuff.c
    src/jcmainct.c
    src/jcmarker.c
    src/jcmaster.c
    src/jcomapi.c
    src/jcparam.c
    src/jcprepct.c
    src/jcsample.c
    src/jcinit.c
    src/jdatadst.c
    src/jerror.c
    src/jfdctint.c
    src/jmemansi.c
    src/jmemmgr.c
    src/jutils.c
    src/rdbmp.c
    src/Rose256_bmp.c

    src/mith/th_lib.c
    src/mith/mith_workload.c
    src/mith/mith_lib.c
    src/mith/th_bignum.c
    src/mith/th_rand.c
    src/mith/th_encode.c

    src/mith/al/th_al.c
    src/mith/al/al_smp.c
    src/mith/al/al_single.c
    src/mith/al/al_file.c
)

target_compile_definitions(app PRIVATE
    SELECT_PRESET_ID=1
    USE_PRESET
)

