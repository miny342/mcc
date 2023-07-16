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
