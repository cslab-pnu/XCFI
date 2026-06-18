#!/bin/bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
XCFI_ROOT="$SCRIPT_DIR"

# build llvm
cd XCFI-LLVM

cmake -S llvm -B build -G Ninja \
  -DLLVM_ENABLE_PROJECTS="clang;lld;compiler-rt" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="ARM;X86" \
  -DBUILD_SHARED_LIBS=ON \
  -DCLANG_ENABLE_OPAQUE_POINTERS=OFF \
  -DCMAKE_INSTALL_PREFIX=$XCFI_ROOT/llvm-install

cd build

cmake --build . --target install -j$(nproc)

export PATH=$XCFI_ROOT/llvm-install/bin:$PATH

# build compiler-rt
cd ../
mkdir build-compiler-rt
cd build-compiler-rt

cmake -G Ninja -DCMAKE_C_COMPILER=$XCFI_ROOT/llvm-install/bin/clang \
    -DCMAKE_CXX_COMPILER=$XCFI_ROOT/llvm-install/bin/clang++ \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_BAREMETAL_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_CONFIG_PATH=$XCFI_ROOT/llvm-install/bin/llvm-config \
    -DCMAKE_INSTALL_PREFIX=$XCFI_ROOT/arm-compiler-rt/xcfi \
    -DLLVM_ENABLE_PROJECTS="compiler-rt" \
    -DCOMPILER_RT_OS_DIR="baremetal" \
    -DCMAKE_C_COMPILER_TARGET=armv8m.main-none-eabi \
    -DLLVM_DEFAULT_TARGET_TRIPLE=armv8m.main-none-eabi \
    -DCMAKE_C_FLAGS="-mthumb -march=armv8-m.main --target=armv8m.main-none-eabi -mcpu=cortex-m85+fp -mfloat-abi=hard -O2 -fno-jump-tables -mbranch-protection=bti+pac-ret -mllvm -enable-XCFI=all" \
    -DCMAKE_ASM_FLAGS="-mthumb -march=armv8-m.main --target=armv8m.main-none-eabi -mcpu=cortex-m85+fp -mfloat-abi=hard -O2 -mbranch-protection=bti+pac-ret -mllvm -enable-XCFI=all" \
    ../compiler-rt

cmake --build . --clean-first --target install -j

cmake -G Ninja -DCMAKE_C_COMPILER=$XCFI_ROOT/llvm-install/bin/clang \
    -DCMAKE_CXX_COMPILER=$XCFI_ROOT/llvm-install/bin/clang++ \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_BAREMETAL_BUILD=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_CONFIG_PATH=$XCFI_ROOT/llvm-install/bin/llvm-config \
    -DCMAKE_INSTALL_PREFIX=$XCFI_ROOT/arm-compiler-rt/base \
    -DLLVM_ENABLE_PROJECTS="compiler-rt" \
    -DCOMPILER_RT_OS_DIR="baremetal" \
    -DCMAKE_C_COMPILER_TARGET=armv8m.main-none-eabi \
    -DLLVM_DEFAULT_TARGET_TRIPLE=armv8m.main-none-eabi \
    -DCMAKE_C_FLAGS="-mthumb -march=armv8-m.main --target=armv8m.main-none-eabi -mcpu=cortex-m85+fp -mfloat-abi=hard -O2 -fno-jump-tables" \
    -DCMAKE_ASM_FLAGS="-mthumb -march=armv8-m.main --target=armv8m.main-none-eabi -mcpu=cortex-m85+fp -mfloat-abi=hard -O2" \
    ../compiler-rt

cmake --build . --clean-first --target install -j

#cd ../../arm-compiler-rt/xcfi
#cp -r ./lib/ $XCFI_ROOT/llvm-install/lib/clang/21/
#cp -r ./include/ $XCFI_ROOT/llvm-install/include/clang/21/
