#!/usr/bin/env bash
# Replace Zephyr CMSIS module with CMSIS v6.1.0

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WORKSPACE_ROOT="$SCRIPT_DIR/.."
CMSIS_DIR="$WORKSPACE_ROOT/modules/hal/cmsis/CMSIS"
TMP_DIR="$WORKSPACE_ROOT/cmsis_tmp"
ZIP_FILE="$WORKSPACE_ROOT/CMSIS_6.1.0.zip"
CMSIS_URL="https://github.com/ARM-software/CMSIS_6/archive/refs/tags/v6.1.0.zip"

echo "[INFO] Workspace root: $WORKSPACE_ROOT"
echo "[INFO] Target CMSIS dir: $CMSIS_DIR"

mkdir -p "$TMP_DIR"
echo "[INFO] Downloading CMSIS v6.1.0..."
wget -q "$CMSIS_URL" -O "$ZIP_FILE"

echo "[INFO] Extracting CMSIS..."
unzip -q -o "$ZIP_FILE" -d "$TMP_DIR"

echo "[INFO] Replacing $CMSIS_DIR with CMSIS v6.1.0"
rm -rf "$CMSIS_DIR"
mv "$TMP_DIR"/CMSIS_6-6.1.0/CMSIS "$CMSIS_DIR"

cat > "$CMSIS_DIR/CMakeLists.txt" <<'EOF'
# SPDX-License-Identifier: Apache-2.0
add_subdirectory_ifdef(CONFIG_HAS_CMSIS_CORE_M Core)
add_subdirectory_ifdef(CONFIG_HAS_CMSIS_CORE_A Core_A)
add_subdirectory_ifdef(CONFIG_HAS_CMSIS_CORE_R Core_R)
EOF

cat > "$CMSIS_DIR/Core/CMakeLists.txt" <<'EOF'
# SPDX-License-Identifier: Apache-2.0
zephyr_include_directories(Include)
zephyr_compile_definitions(__PROGRAM_START)
EOF

echo "[INFO] Cleaning temporary files..."
rm -rf "$TMP_DIR"
rm -f "$ZIP_FILE"

echo "[INFO] CMSIS v6.1.0 setup complete!"

