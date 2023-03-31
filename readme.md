# mycmmi: My C-- interpreter

一个自举的 C 子集解释器，基于 `c4`

备战面试，简单复习编译原理过程中的产物

## 使用：

```shell
clang -o mycmmi ./mycmmi.c
./mycmmi ./fib.c

./mycmmi ./mycmmi.c ./mycmmi.c ./fib.c
```

## 支持的操作

```
if, else, else if
while
+ - * / == != < > <= >= 
* & (取地址，解引用) [] (运算符)
函数调用
open, close, printf, read, malloc, memset, memcmp
```