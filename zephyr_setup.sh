#!/bin/bash

python3 -m venv ./.XCFI-venv
source ./.XCFI-venv/bin/activate
pip install west

cd XCFI-zephyr
#git checkout artifact
west init -l .
west update
west zephyr-export
pip install -r scripts/requirements.txt

./setup-cmsis.sh
./patch_picolibc.sh
