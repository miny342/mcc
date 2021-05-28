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

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 42 "12 + 34 - 4;"
assert 47 "5+6*7;"
assert 15 "5* (9-6 );"
assert 4 "(3 + 5) / 2;"
assert 9 "+4 /-2*- 4+ 1;"
assert 1 "3 == 1 != 1;"
assert 1 "(1 < 1 + 2) + 1 >= 2;"
assert 2 "(1 > 1) + 2;"
assert 1 "1 + 1 == 2;"
assert 2 "a = 1;b = a + 1; b;"
assert 3 "foo = 1; ba = 3; n2a = 7; n2a-foo-ba;"
assert 4 "return 4; return 3;"
assert 10 "i = 0; while(i < 10) i = i + 1; return i;"
assert 11 "i = 1; while(i < 10) if(i < 5) i = i + 2; else i = i + 3; return i;"
assert 55 "tmp = 0; for(i = 0; i <= 10; i = i + 1) tmp = tmp + i; return tmp;"
assert 75 "width = 5; height=10; ans = 0; for(i = 1; i <= width; i = i + 1) for(j = 1; j <= height; j = j + 1) if(j > height / 2) ans = ans + 2; else ans = ans + 1; return ans;"
assert 233 "
fib = 13;
if(fib < 0) return 0;
current = 0;
next = 1;
for(i = 0; i < fib; i = i + 1) {
  tmp = current;
  current = next;
  next = next + tmp;
}
return current;
"

echo OK
