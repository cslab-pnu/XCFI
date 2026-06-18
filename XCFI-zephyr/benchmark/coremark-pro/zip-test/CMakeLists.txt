# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zip-test)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

target_sources(app PRIVATE src/zip-test.c
    src/zip_darkmark.c
    src/adler32.c
    src/compress.c
    src/crc32.c
    src/deflate.c
    src/gzclose.c
    src/gzlib.c
    src/infback.c
    src/inffast.c
    src/inflate.c
    src/inftrees.c
    src/trees.c
    src/uncompr.c
    src/zutil.c
    src/stubs.c

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
    MITH_MEMORY_ONLY_VERSION
    ZLIB_COMPAT_ALL
    ZLIB_ANSI
)

