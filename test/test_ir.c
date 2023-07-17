int t0() {
    int foo = 1;
    int ba = 3;
    int n2a = 7;
    return n2a-foo-ba;
}

int t1() {
    int i;
    i = 1;
    while(i < 10)
        if(i < 5) i = i + 2;
        else i = i + 3;
    return i;
}

int t2() {
    int tmp;
    int i;
    tmp = 0;
    for(i = 0; i <= 10; i = i + 1)
        tmp = tmp + i;
    return tmp;
}

int t3() {
    int (*f)(int, int, int, int, int, int, int, int) = t2;
    f(1, 2, 3, 4, 5, 6, 7, 8);
}

int t29_a;
int t29_b[10];
int t29_foo() {
    return t29_b[t29_a];
}

int t4() {
    char *a = "aa";
    char b[] = "bb";
}

int t5(int a, int b) {
    int x = a && b;
    int y = a || b;
}
