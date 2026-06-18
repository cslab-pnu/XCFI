#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

# MPS3 USB mass-storage volume root.
# Default is the fixed mount provisioned for artifact evaluation;
# override with MPS3_MOUNT=... if your board is mounted elsewhere.
MPS3="${MPS3_MOUNT:-/mnt/mps3}"

SOFTWARE="$MPS3/SOFTWARE"
IMAGES_TXT="$MPS3/MB/HBI0309C/AN555/images.txt"

# Build outputs to flash (TF-M: bl2 + tfm_s + zephyr_ns).
IMAGES_SRC="$SCRIPT_DIR/images_with_tfm.txt"
BL2_SRC="$SCRIPT_DIR/build/tfm/bin/bl2.bin"
TFM_S_SRC="$SCRIPT_DIR/build/zephyr/tfm_s_signed.bin"
ZEPHYR_SRC="$SCRIPT_DIR/build/zephyr/zephyr_ns_signed.bin"

# 1. Ensure the board is mounted and writable (no sudo required once the
#    fixed mount is set up with the evaluation user's uid/gid).
if [ ! -d "$SOFTWARE" ] || [ ! -w "$SOFTWARE" ]; then
    echo "error: MPS3 not mounted/writable at '$MPS3'." >&2
    echo "       check the board connection and the fixed mount" >&2
    echo "       (set MPS3_MOUNT=... to override)." >&2
    exit 1
fi
if [ ! -d "$(dirname "$IMAGES_TXT")" ]; then
    echo "error: board config dir '$(dirname "$IMAGES_TXT")' not found." >&2
    exit 1
fi

# 2. Verify all build outputs exist BEFORE touching the board, so a missing
#    or unbuilt image never leaves the board half-flashed.
for f in "$IMAGES_SRC" "$BL2_SRC" "$TFM_S_SRC" "$ZEPHYR_SRC"; do
    if [ ! -f "$f" ]; then
        echo "error: missing build output '$f'. did the build run?" >&2
        exit 1
    fi
done

# 3. Flash: board config + the three images.
cp "$IMAGES_SRC" "$IMAGES_TXT"
rm -f "$SOFTWARE/bl2.bin" "$SOFTWARE/tfm_s.bin" "$SOFTWARE/zephyr.bin"
echo "original bin files deleted"
cp "$BL2_SRC"    "$SOFTWARE/bl2.bin"
cp "$TFM_S_SRC"  "$SOFTWARE/tfm_s.bin"
cp "$ZEPHYR_SRC" "$SOFTWARE/zephyr.bin"
echo "new bin files copied"

# 4. Flush to the USB mass-storage device so the board sees a complete
#    image set before it is rebooted.
sync
echo "flash complete. now reboot the board to load the new images."
