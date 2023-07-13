#!/bin/bash
set -e
# cc -E test/test.c > test/tmp.c
# python3 skip.py
./mcc test/test.c > tmp.s
cc -o tmp tmp.s
echo "start run"
./tmp
