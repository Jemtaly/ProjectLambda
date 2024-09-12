#!/usr/bin/sh

for f in *.cpp; do
    make SOURCE=$f
    make SOURCE=$f USE_GMP=1
done
