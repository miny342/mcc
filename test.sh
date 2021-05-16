#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./mcc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
      echo "$input ==> $actual"
    else
      echo "$input ==> $expected expected, but got $actual"
      exit 1
    fi
}

assert 0 0
assert 42 42
assert 21 "5+20-4"
assert 42 "12 + 34 - 4"
assert 47 "5+6*7"
assert 15 "5* (9-6 )"
assert 4 "(3 + 5) / 2"
assert 9 "+4 /-2*- 4+ 1"
assert 1 "3 == 1 != 1"
assert 1 "(1 < 1 + 2) + 1 >= 2"
assert 2 "(1 > 1) + 2"
assert 1 "1 + 1 == 2"

echo OK
