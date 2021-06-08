#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./mcc "$input" > tmp.s
    cc -o tmp tmp.s support.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
      echo "$input ==> $actual"
    else
      echo "$input ==> $expected expected, but got $actual"
      exit 1
    fi
}

assert 0 "main() {0;}"
assert 42 "main() {42;}"
assert 21 "main() {5+20-4;}"
assert 42 "main() {12 + 34 - 4;}"
assert 47 "main() {5+6*7;}"
assert 15 "main() {5* (9-6 );}"
assert 4 "main() {(3 + 5) / 2;}"
assert 9 "main() {+4 /-2*- 4+ 1;}"
assert 1 "main() {3 == 1 != 1;}"
assert 1 "main() {(1 < 1 + 2) + 1 >= 2;}"
assert 2 "main() {(1 > 1) + 2;}"
assert 1 "main() {1 + 1 == 2;}"
assert 2 "main() {a = 1;b = a + 1; b;}"
assert 3 "main() {foo = 1; ba = 3; n2a = 7; n2a-foo-ba;}"
assert 4 "main() {return 4; return 3;}"
assert 10 "main() {i = 0; while(i < 10) i = i + 1; return i;}"
assert 11 "main() {i = 1; while(i < 10) if(i < 5) i = i + 2; else i = i + 3; return i;}"
assert 55 "main() {tmp = 0; for(i = 0; i <= 10; i = i + 1) tmp = tmp + i; return tmp;}"
assert 75 "main() {width = 5; height=10; ans = 0; for(i = 1; i <= width; i = i + 1) for(j = 1; j <= height; j = j + 1) if(j > height / 2) ans = ans + 2; else ans = ans + 1; return ans;}"
assert 233 "
main() {
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
}
"
# assert 0 "i = 11; foo(1 == 1,2,i-8,2+2,1+2+2,6,7,8);"
assert 233 "main() { return fib(13); } fib(i) { if(i <= 0) return 0; if(i == 1) return 1; if(i == 2) return 1; return fib(i - 1) + fib(i - 2); }"
# assert 1 "main() { return foo(3); } foo(i) { if(i == 0) return 0; if(i == 1) return 1 ; return foo(i - 1);}"
# assert 0 "main() {
#   foo(1,2,3,4,5,6,7,8);
# }
# foo(a,b,c,d,e,f,g,h) {
#   i = 9;
#   j = 10;
#   print(i);
#   print(j);
#   print(a);
#   print(b);
#   print(c);
#   print(d);
#   print(e);
#   print(f);
#   print(g);
#   print(h);
#   return 0;
# }"
assert 3 "main() {x = 3; y = &x; return *y;}"
assert 3 "main() {x = 3; y = 5; z = &y + 8; return *z;}"

echo OK
