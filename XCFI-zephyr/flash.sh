#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

# MPS3 USB mass-storage volume root.
# Default is the fixed mount provisioned for artifact evaluation;
# override with MPS3_MOUNT=... if your board is mounted elsewhere.
MPS3="${MPS3_MOUNT:-/mnt/mps3}"

SOFTWARE="$MPS3/SOFTWARE"
IMAGES_TXT="$MPS3/MB/HBI0309C/AN555/images.txt"

# Build output to flash (single image: zephyr only, no TF-M).
IMAGES_SRC="$SCRIPT_DIR/images_single.txt"
ZEPHYR_SRC="$SCRIPT_DIR/build/zephyr/zephyr.bin"

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

# 2. Verify the build output exists BEFORE touching the board.
for f in "$IMAGES_SRC" "$ZEPHYR_SRC"; do
    if [ ! -f "$f" ]; then
        echo "error: missing build output '$f'. did the build run?" >&2
        exit 1
    fi
done

# 3. Flash: board config + the single image.
cp "$IMAGES_SRC" "$IMAGES_TXT"
rm -f "$SOFTWARE/zephyr.bin"
echo "original bin file deleted"
cp "$ZEPHYR_SRC" "$SOFTWARE/zephyr.bin"
echo "new bin file copied"

# 4. Flush to the USB mass-storage device so the board sees a complete
#    image before it is rebooted.
sync
echo "flash complete. now reboot the board to load the new image."
