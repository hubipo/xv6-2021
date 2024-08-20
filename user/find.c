#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 格式化文件名，用于比较
char* fmtname(char *path) {
    static char buf[DIRSIZ+1];
    char *p;

    // 查找路径中最后一个斜杠后的部分
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // 返回文件名
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    buf[strlen(p)] = 0;
    return buf;
}


// 递归查找函数
void find(char *path, char *target) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    // 打开目录
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // 获取文件状态
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 如果是文件，直接检查名字是否匹配
    if (st.type == T_FILE) {
        if (strcmp(fmtname(path), target) == 0) {
            printf("%s\n", path);
        }
        close(fd);
        return;
    }

    // 如果是目录，开始遍历
    if (st.type == T_DIR) {
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            printf("find: path too long\n");
            close(fd);
            return;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        // 遍历目录中的每个条目
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;

            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if (stat(buf, &st) < 0) {
                printf("find: cannot stat %s\n", buf);
                continue;
            }

            // 如果找到匹配的文件，打印其路径
            if (st.type == T_FILE && strcmp(de.name, target) == 0) {
                printf("%s\n", buf);
            }

            // 如果是子目录，且不是 "." 或 ".."，则递归查找
            if (st.type == T_DIR && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0) {
                find(buf, target);
            }
        }
    }

    close(fd);
}


// 主函数
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: find <directory> <filename>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}
