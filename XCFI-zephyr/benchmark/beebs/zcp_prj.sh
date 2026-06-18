#!/bin/bash

dir=`ls`

cur="./"
prev="./"
for i in $dir
do
    cur="./${i}"
    echo "cur ${i}, prev ${i}"
    cp ${prev}/prj.conf ${cur}/prj.conf
    echo "copy done!"
    prev="./${i}"
done
