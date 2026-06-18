#!/bin/bash
set -euo pipefail

PICOLIBC_CMAKE="$PWD/../modules/lib/picolibc/CMakeLists.txt"

if [ ! -f "$PICOLIBC_CMAKE" ]; then
    echo "Error: cannot find $PICOLIBC_CMAKE"
    exit 1
fi

python3 - << 'PY' "$PICOLIBC_CMAKE"
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()

old = "target_compile_options(c PRIVATE ${PICOLIBC_COMPILE_OPTIONS})"

patch = """list(REMOVE_ITEM PICOLIBC_COMPILE_OPTIONS CHECK_ONLY SEARCH_PARENTS VERSION_CHECK)
target_compile_options(c PRIVATE ${PICOLIBC_COMPILE_OPTIONS})"""

if "list(REMOVE_ITEM PICOLIBC_COMPILE_OPTIONS CHECK_ONLY SEARCH_PARENTS VERSION_CHECK)" in text:
    print(f"Already patched: {path}")
    sys.exit(0)

if old not in text:
    print(f"Error: target line not found in {path}")
    sys.exit(1)

text = text.replace(old, patch, 1)
path.write_text(text)

print(f"Patched: {path}")
PY

