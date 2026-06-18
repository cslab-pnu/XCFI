#!/bin/bash

dir=`ls`

cur="./"
prev="./"
for i in $dir
do
    cur="./${i}"
    echo "cur ${i}, prev ${i}"
    cp ${prev}/src/main.c ${cur}/src/main.c
    cp ${prev}/src/support.h ${cur}/src/support.h
    echo "copy done!"
    prev="./${i}"
done

for dir in */; do
    dirname="${dir%/}"

    if [ -f "$dir/src/main.c" ]; then
        echo "Processing $dir/src/main.c: replacing 'aha-compress' with $dirname"
        sed -i "s/aha-compress/$dirname/g" "$dir/src/main.c"
    else
        echo "Skipping $dir: main.c not found."
    fi
done
