# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nettle-arcfour)
add_compile_definitions(PROJECT_NAME="${PROJECT_NAME}")

target_sources(app PRIVATE src/main.c src/arcfour.c)
