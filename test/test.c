#define assert(name, expr, expect) \
int name ## _assert () expr \
int name () {\
    int name ## _i;\
    name ## _i = name ## _assert ();\
    if(name ## _i != (expect)) {\
        printf("%s: expected %d, found %d\n", #name, (expect), name ## _i);\
        exit(1);\
    } else printf("ok %s\n", #name);\
}

assert(t0, {}, 0)
assert(t1, {return 42;}, 42)
assert(t2, {return 5+20-4;}, 21)
assert(t3, {return 12 + 34 - 4;}, 42)
assert(t4, {return 5+6*7;}, 47)
assert(t5, {return 5* (9-6 );}, 15)
assert(t6, {return (3 + 5) / 2;}, 4)
assert(t7, {return +4 /-2*- 4+ 1;}, 9)
assert(t8, {return 3 == 1 != 1;}, 1)
assert(t9, {return (1 < 1 + 2) + 1 >= 2;}, 1)
assert(t10, {return (1 > 1) + 2;}, 2)
assert(t11, {return 1 + 1 == 2;}, 1)
assert(t12, {int a; a = 1; int b; b = a + 1; return b;}, 2)
assert(t13, {int foo; int ba; int n2a; foo = 1; ba = 3; n2a = 7; return n2a-foo-ba;}, 3)
assert(t14, {return 4; return 3;}, 4)
assert(t15, {int i; i = 0; while(i < 10) i = i + 1; return i;}, 10)
assert(t16, {int i; i = 1; while(i < 10) if(i < 5) i = i + 2; else i = i + 3; return i;}, 11)
assert(t17, {int tmp; int i; tmp = 0; for(i = 0; i <= 10; i = i + 1) tmp = tmp + i; return tmp;}, 55)
assert(t18, {int width; int height; int ans; int i; int j; width = 5; height=10; ans = 0; for(i = 1; i <= width; i = i + 1) for(j = 1; j <= height; j = j + 1) if(j > height / 2) ans = ans + 2; else ans = ans + 1; return ans;}, 75)
assert(t19, {
    int fib;
    int current;
    int next;
    int tmp;
    int i;
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
}, 233)

assert(t20, {return t20_fib(13);}, 233)
int t20_fib(int i) {
    if (i <= 0) return 0;
    if (i == 1) return 1;
    if (i == 2) return 1;
    return t20_fib(i - 1) + t20_fib(i - 2);
}

assert(t21, {int x; int *y; x = 3; y = &x; return *y;}, 3)
assert(t22, {int x; int y; int *z; x = 3; y = 5; z = &y + 2; return *z;}, 3)
assert(t23, {int x; int *y; y = &x; *y = 5; return x;}, 5)
assert(t24, {
    int *ptr;
    ptr = alloc4(1,2,3,4);
    print(*ptr);
    print(*(ptr + 1));
    print(*(ptr + 2));
    print(*(ptr + 3));
    return *(ptr + 2);
}, 3)
assert(t25, {
    int p;
    int *q;
    print(sizeof p);
    print(sizeof(q));

    print(sizeof(p + 3));
    print(sizeof(q + 3));
    print(sizeof(*q));

    print(sizeof(&p));

    print(sizeof(1));
    print(sizeof(sizeof(1)));

    return 0;
}, 0)
assert(t26, {
  int a[3];
  return sizeof(a) * sizeof(a + 1);
  // 12 * 8
}, 96)
assert(t27, {
  int a[2];
  *(a + 1) = 1;
  *(a + 0) = 2;
  int *p;
  p = a;
  return *p + *(p + 1);  // -> 3
}, 3)
assert(t28, {
  int a[13];
  a[0] = 1;
  a[1] = 1;
  int i;
  for(i = 2; i < 13; i = i + 1) {
    a[i] = (i - 1)[a] + a[i - 2];
  }
  return 12[a];
}, 233)

int t29_a;
int t29_b[10];
int t29_foo() {
    return t29_b[t29_a];
}
assert(t29, {
  t29_a = 5;
  t29_b[5] = 4;
  return t29_foo() * 3;
}, 12)

assert(t30, {
  char x[3];
  x[0] = -1;
  x[1] = 2;
  int y;
  y = 4;
  return x[0] + y;  // → 3
}, 3)
assert(t31, {
  char *c;
  char *d;
  c = "anna";
  d = "yuriko";
  printf("%s %s %d\n", c, d, 1000);
  return 1;
}, 1)
assert(t32, {
    int i;
    i = 0;
    for(; i < 4; i = i + 1) {}
}, 0)

int main() {
    t0();t1();t2();t3();t4();t5();t6();t7();t8();t9();t10();
    t11();t12();t13();t14();t15();t16();t17();t18();t19();t20();
    t21();t22();t23();t24();t25();t26();t27();t28();t29();t30();
    t31();t32();

    printf("OK\n");
}
