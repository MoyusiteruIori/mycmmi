#include <stdio.h>
#include <stdlib.h>

int fib(int i) {
    if (i <= 1) {
        return 1;
    }
    return fib(i - 1) + fib(i - 2);
}

int main() {
    int i, *arr;
    i = 0;
    arr = (int*)malloc(40);
    while (i <= 9) {
        arr[i] = fib(i);
        i = i + 1;
    }
    i--;
    while (i >= 0) {
        printf("fib(%d)=%d\n", i, arr[i]);
        i--;
    }
    return 0;
}