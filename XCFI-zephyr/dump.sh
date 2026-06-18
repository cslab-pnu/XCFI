#!/bin/sh

rm -f dump
llvm-objdump -d --mattr=+pacbti -S ./build/zephyr/zephyr.elf > dump
