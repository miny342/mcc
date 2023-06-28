void *(*(*f(int a))(int))(int);
void (*signal(int, void (*)(int)))(int);
int (*(*testf(void))[10])(void);
int c[12][34];
int (c2[12])[];

int printf(char *s, ...);
void exit(int status);
void print(int i);
int *alloc4(int a, int b, int c, int d);

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

int t20_fib(int i) {
    if (i <= 0) return 0;
    if (i == 1) return 1;
    if (i == 2) return 1;
    return t20_fib(i - 1) + t20_fib(i - 2);
}
assert(t20, {return t20_fib(13);}, 233)


assert(t21, {int x; int *y; x = 3; y = &x; return *y;}, 3)
// assert(t22, {int x; int y; int *z; x = 3; y = 5; z = &y + 2; return *z;}, 3)
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
  x[0] = -1; // 255
  x[1] = 2;
  int y;
  y = 4;
  return x[0] + y;  // → 259
}, 259)
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

char mul(char l, char r) {
    char re;
    char v;
    re = 0;
    v = r;
    while(l) {
        if (l & 1) {
            re = re ^ v;
        }
        if (v & (1 << 7)) {
            v = (v << 1) ^ 27;
        } else {
            v = v << 1;
        }
        l = l >> 1;
    }
    return re;
}
char rotl(char v, int c) {
    return v << c | v >> (8 - c);
}
assert(t33, {
    char expt[256];
    char logt[256];
    char v;
    v = 1;
    int i;
    for(i = 0; i < 256; i = i + 1) {
        expt[i] = v;
        logt[v] = i;
        v = mul(3, v);
    }
    char sb[256];
    char inv;
    sb[0] = 99;
    for(i = 1; i < 256; i = i + 1) {
        inv = expt[255 - logt[i]];
        sb[i] = inv ^ rotl(inv, 1) ^ rotl(inv, 2) ^ rotl(inv, 3) ^ rotl(inv, 4) ^ 99;
    }
    return sb[134];
}, 68)  // unsigned char型とみているので問題が出ない
assert(t34, {
    int mod;
    mod = 1000000007;
    int r;
    int x;
    int n;
    n = 1000;
    r = 1;
    x = 2;
    while (n) {
        if (n & 1) {
            r = r * x % mod;
        }
        x = x * x % mod;
        n = n >> 1;
    }
    return r;
}, 688423210) // 裏ではすべて64bitレジスタで計算するため、オーバーフローしない
assert(t35, {return 6 && 8;}, 1)
assert(t36, {return 0 && 2;}, 0)
assert(t37, {return 3 && 0;}, 0)
assert(t38, {return 0 && 0;}, 0)
assert(t39, {return 7 || 0;}, 1)
assert(t40, {return 0 || 3;}, 1)
assert(t41, {return 4 || 2;}, 1)
assert(t42, {return 0 || 0;}, 0)
assert(t43, {int i; i = -1; return i >> 1;}, -1)
assert(t44, {char i; i = -1; return i >> 1;}, 127)
assert(t45, {int i = 2323; int *j = &i; return *j;}, 2323)

int _t46() {
    int i[] = {3, 1, 4};  // ここでexpr句が切れるため外に出してある
    return i[2];
}
assert(t46, {return _t46();}, 4)
assert(t47, {char a[5] = "foo"; printf("%s\n", a); return a[2];}, 111)
assert(t48, {char b[] = "something"; printf("%s\n", b); return b[3];}, 101)
assert(t49, {char *c = "???"; printf("%s\n", c); return c[1];}, 63)
assert(t50, {int a = 1; a <<= 2; a >>= 1; a++; a *= 4; a = ~a; a = -a; int b = a--; return b;}, 13)
assert(t51, {int a = 1; int b = ++a; b <<= 3; b %= 13; b /= 2; return b;}, 1)
assert(t52, {int i = 1; int o = 0;return !((i && o) || (i && i));}, 0)
assert(t53, {
    int i;
    for (i = 0; i < 100; i++) {
        if (i == 50) {
            break;
        } else {
            continue;
        }
        printf("do not show\n");
    }
    return i;
}, 50)
assert(t54, {
    int i;
    int flag = 0;
    int j = 0;
    while(1) {
        for (i = 0; i < 100; i++) {
            if (200 - j == i) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            break;
        }
        j++;
    }
    return j;
}, 101)

int _t55(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    printf("%d %d %d %d %d %d %d %d %d %d\n", a, b, c, d, e, f, g, h, i, j);
    return a + b + c + d + e + f + g + h + i + j;
}
assert(t55, {return _t55(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);}, 55)
assert(t56, {
    int (*p)(int, int, int, int, int, int, int, int, int, int) = _t55;
    printf("%p\n", p);
    return p(2, 4, 6, 8, 10, 12, 14, 16, 18, 20);
}, 110)
assert(t57, {
    int *p;
    int *q;
    {
        int i;
        i = 1;
        p = &i;
    }
    // i は使えない
    for(int i = 0; i < 1; i++) q = &i;
    // 同様
    return p == q; // 内部的にはアドレスが同じになっている
}, 1);
assert(t58, {
    int a[10][10];
    int (*b)[10][10] = &a;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            (*b)[i][j] = i + j;
        }
    }
    return a[3][5];
}, 8);
assert(t59, {return sizeof(int (*[4])()) + sizeof(char[11][11]);}, 32 + 121);

int a = 1;
int *d[10] = { &a + 1, &a + 2 };
int *b = &a;
char k[] = "anna";
char kl[10] = "and";
char *p = "pic";
int (*ffn)() = t0_assert + 1;

int main() {
    t0();t1();t2();t3();t4();t5();t6();t7();t8();t9();t10();
    t11();t12();t13();t14();t15();t16();t17();t18();t19();t20();
    t21();t23();t24();t25();t26();t27();t28();t29();t30();
    t31();t32();t33();t34();t35();t36();t37();t38();t39();t40();
    t41();t42();t43();t44();t45();t46();t47();t48();t49();t50();
    t51();t52();t53();t54();t55();t56();t57();t58();t59();

    printf("OK\n");
}
