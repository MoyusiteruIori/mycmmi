#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#define int long long

int token;              // 当前 token
char *src, *old_src;    // 源代码
int poolsize;           // text/data/stack 段的默认大小
int line;               // 行号

// +------------------+
// |    stack   |     |      high address
// |    ...     v     |
// |                  |
// |                  |
// |                  |
// |                  |
// |    ...     ^     |
// |    heap    |     |
// +------------------+
// | bss  segment     |
// +------------------+
// | data segment     |
// +------------------+
// | text segment     |      low address
// +------------------+

int *text,
    *old_text,
    *stack;
char *data;

int *pc,    
    *bp,    // 基址指针，指向栈顶某些位置
    *sp,    // 栈顶
     ax,    // 通用寄存器
     cycle;

enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
       OR,  XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD ,
       OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT };

// tokens 和类别，运算符按优先级升序排列
enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// struct identifier
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

int token_val;      // 当前 token 的值
int *current_id,    // 当前分析的 id
    *symbols;       // 符号表


// 变量/函数的类型
enum { CHAR, INT, PTR };
int *idmain;                  // main 函数

// 词法分析，获取下一个 token
void next() {
    char *last_pos;
    int hash;

    while (token = *src) {
        src++;
        if (token == '\n') {
            ++line;
        }
        else if (token == '#') {
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {

            // 解析标识符
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }

            // 查找已存在的标识符
            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
                    // 找到，返回
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }

            // 存储新的 id
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        }
        else if (token >= '0' && token <= '9') {
            // 解析数字, 包括三种: 十进制(123) 十六进制(0x123) 八进制(017)
            token_val = token - '0';
            if (token_val > 0) {
                // 十进制，开头为[1-9]
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
            } else {
                // 开头为 0
                if (*src == 'x' || *src == 'X') {
                    // 十六进制
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // 八进制
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }
        else if (token == '"' || token == '\'') {
            // 解析字符串常量, 转义字符只支持 ‘\n’
            // 将字符串常量存入 data 段中
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    // 转义字符
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            // 如果是单个字符，返回 Num token
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }

            return;
        }
        else if (token == '/') {
            if (*src == '/') {
                // 跳过注释
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                // 除法运算符
                token = Div;
                return;
            }
        }
        else if (token == '=') {
            // 解析 '==' '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // 解析 '+' '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // 解析 '-' '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // 解析 '!='
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // 解析 '<=', '<<' '<'
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // 解析 '>=', '>>' '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // 解析 '|' '||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // 解析 '&' '&&'
            if (*src == '&') {
                src ++;
                token = Lan;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // 直接返回字符作为 token
            return;
        }
    }

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
    int op, *tmp;
    while (1) {
        op = *pc++; // get next operation code

        if (op == IMM)       {ax = *pc++;}
        else if (op == LC)   {ax = *(char *)ax;}
        else if (op == LI)   {ax = *(int *)ax;}
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}
        else if (op == SI)   {*(int *)*sp++ = ax;}
        else if (op == PUSH) {*--sp = ax;}
        else if (op == JMP)  {pc = (int *)*pc;}
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;}
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}
        else if (op == ADJ)  {sp = sp + *pc++;}
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;}
        else if (op == LEA)  {ax = (int)(bp + *pc++);}
        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;
        else if (op == EXIT) { printf("exit(%d)\n", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
        else {
            printf("unknown instruction:%d\n", op);
            return -1;
        }
    }
    return 0;
}

#undef int


int main(int argc, char **argv) {

#define int long long
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

    // 为虚拟机分配内存
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }

    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }

    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

     // 保留字加入符号表
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
    }

    // 库函数加入符号表
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next(); current_id[Token] = Char; // void 类型
    next(); idmain = current_id; // main 函数


    program();
    return eval();

#undef int 
}