#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token;              // 当前 token
char *src, *old_src;    // 源代码
int poolsize;           // text/data/stack 段的默认大小
int line;               // 行号

// 词法分析，获取下一个 token
void next() {
    token = *src++;
}

// 解析表达式
void expression(int level) {

}

// 语法分析的入口
void program() {
    next();
    while (token > 0) {
        printf("token is: %c\n", token);
        next();
    }
}

// 虚拟机入口
int eval() {
    return 0;
}

int main(int argc, char **argv) {
    int i, fd;

    argc--;
    argv++;

    poolsize = 256 * 1024;
    line = 1;

    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }

    // 读取源文件
    if ((i = read(fd, src, poolsize - 1)) <= 0) {
        printf("read() returned %d\n", i);
    }
    src[i] = 0;
    close(fd);

    program();
    return eval();
}