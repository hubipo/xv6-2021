#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

// 执行指定的命令，传入参数列表
void xargs_exec(char* program, char** paraments) {
    if (fork()== 0) {
       // 子进程执行命令
        if (exec(program, paraments) == -1) {
            // 如果 exec 失败，输出错误信息
            fprintf(2, "xargs: Error exec %s\n", program);
            exit(1);
        }
    } else {
        wait(0);
    }
}


// xargs 函数，读取标准输入并将其作为参数传递给指定命令执行
void xargs(char** first_arg, int size, char* program_name) {
    char buf[1024];  // 用于存储从标准输入读取的行
    char *arg[MAXARG];  // 存储命令及其参数的数组
    int m = 0;  // 当前读取的字符位置索引

    // 逐字节读取标准输入的数据
    while (read(0, buf + m, 1) == 1) {
        // 如果读取的数据超过缓冲区大小，输出错误信息并退出
        if (m >= sizeof(buf)) {
            fprintf(2, "xargs: arguments too long.\n");
            exit(1);
        }

        // 如果读取到换行符，表示一行输入结束
        if (buf[m] == '\n') {
            buf[m] = '\0';  // 将换行符替换为字符串终止符

            // 复制初始参数到 arg 数组中
            memmove(arg, first_arg, sizeof(*first_arg) * size);

            // 设置参数的起始索引，初始参数已经复制完毕
            int argIndex = size;
            
            // 如果没有初始参数，设置第一个参数为程序名
            if (argIndex == 0) {
                arg[argIndex] = program_name;
                argIndex++;
            }

            // 为新读取的参数分配内存，并将其内容复制到该内存中
            arg[argIndex] = malloc(sizeof(char) * (m + 1));
            memmove(arg[argIndex], buf, m + 1);

            // 设置参数列表的结束符，即 NULL 指针
            arg[argIndex + 1] = 0;

            // 执行命令，将参数传递给命令执行
            xargs_exec(program_name, arg);

            // 释放动态分配的内存
            free(arg[argIndex]);
            
            // 重置索引 m，准备处理下一行输入
            m = 0;
        } else {
            // 如果没有遇到换行符，继续读取下一个字符
            m++;
        }
    }
}


// 主函数，处理命令行参数并调用 xargs 函数
int main(int argc, char* argv[]) {
    char *name = "echo";  // 默认命令为 echo

    // 如果用户提供了命令，将其作为要执行的程序名
    if (argc >= 2) {
        name = argv[1];
    }

    // 调用 xargs 函数，传递命令行参数（不包括程序名）
    xargs(argv + 1, argc - 1, name);

    exit(0);  // 正常退出
}
