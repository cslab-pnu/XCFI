#!/bin/sh

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <project_name> [base/xcfi]"
    exit 1
fi

. ../.XCFI-venv/bin/activate

# --- configure board files for a build WITH TF-M ---
# Force the chosen sram/flash mapping and ROM relocation regardless of current
# values (idempotent; also strips any trailing "// ..." comment a prior run left).
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BOARD_DIR="$SCRIPT_DIR/boards/arm/mps3"
DTS="$BOARD_DIR/mps3_corstone310_an555.dts"
DEFCONFIG="$BOARD_DIR/mps3_corstone310_an555_defconfig"

sed -i -E 's|^([[:space:]]*zephyr,sram = ).*|\1\&sram;|'   "$DTS"
sed -i -E 's|^([[:space:]]*zephyr,flash = ).*|\1\&isram;|' "$DTS"
sed -i -E 's|^(CONFIG_ROMSTART_RELOCATION_ROM=).*|\1y|'    "$DEFCONFIG"

COMPILE_MODE="${2:-xcfi}"
CFLGAS=""

if [ "$COMPILE_MODE" = "base" ]; then
    CFLAGS="$CFLAGS -fno-jump-tables -O2"
    cp -r ../arm-compiler-rt/base/lib ../llvm-install/lib/clang/21/
    cp -r ../arm-compiler-rt/base/include ../llvm-install/lib/clang/21/
    else #xcfi
        CFLAGS="$CFLAGS -mbranch-protection=bti+pac-ret -mllvm -enable-XCFI=all -DENABLE_PACBTI -DENABLE_XCFI -mllvm -enable-arm-trustzone-cfi"
    	cp -r ../arm-compiler-rt/xcfi/lib ../llvm-install/lib/clang/21/
    	cp -r ../arm-compiler-rt/xcfi/include ../llvm-install/lib/clang/21/
fi

if [ "$1" = "samples/tfm_integration/ret2ns" ]; then
    CFLAGS="$CFLAGS -D$3"
fi

# trio-snprintf
#CFLAGS="$CFLAGS -DTRIO_EXTENSION=0 -DTRIO_DEPRECATED=0 -DTRIO_MICROSOFT=0 -DTRIO_ERRORS=0 -DTRIO_FEATURE_FLOAT=0"

# trio-sscanf
#CFLAGS="$CFLAGS -DTRIO_SSCANF -DTRIO_FEATURE_FILE=0 -DTRIO_FEATURE_STDIO=0 -DTRIO_FEATURE_FD=0 -DTRIO-FEATURE_DYNAMICSTRING=0 \
    #-DTRIO_FEATURE_CLOSURE=0 -DTRIO_FEATURE_STRERR=0 -DTRIO_FEATURE_LOCALE=0 -DTRIO_EMBED_NAN=1 -DTRIO_EMBED_STRING=1"

# coremark-pro
#CFLAGS="$CFLAGS -DFAKE_FILEIO=1 -DHAVE_SYS_STAT=0 -DHAVE_SYS_STAT_H=0 -DUSE_SINGLE_CONTEXT=1 -DHAVE_STRDUP=0 -DNO_ALIGNED_ALLOC=1 -DUSE_CLOCK=0 -DHOST_EXAMPLE_CODE=1 -DNEED_MKSTEMP -DSTUB_STAT=1"

#west build -p always -b ek_ra8m1 $1 -DZEPHYR_TOOLCHAIN_VARIANT=llvm -DCMAKE_C_FLAGS="$CFLAGS" \
    #-DCMAKE_ASM_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CFLAGS"
#west flash

echo $CFLAGS
west build -p always -b mps3/corstone310/an555/ns $1 -DZEPHYR_TOOLCHAIN_VARIANT=llvm -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_ASM_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CFLAGS" -DCOMPILE_MODE=${COMPILE_MODE} \
    -DCONFIG_BUILD_WITH_TFM=y
