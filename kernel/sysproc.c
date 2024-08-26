#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

extern pte_t *walk(pagetable_t, uint64, int);

#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 srcva, st;
    int len;
    uint64 buf = 0;
    struct proc *p = myproc();

    // 获取系统调用参数
    if (argaddr(0, &srcva) < 0 || argint(1, &len) < 0 || argaddr(2, &st) < 0) {
        return -1;
    }

    // 参数验证
    if (len <= 0 || len > 64) {
        return -1;
    }

    acquire(&p->lock);  // 锁住进程以确保页表访问的安全性

    for (int i = 0; i < len; i++) {
        pte_t *pte = walk(p->pagetable, srcva + i * PGSIZE, 0);
        
        // 检查 PTE 是否有效和用户访问权限
        if (!pte || !(*pte & PTE_V) || !(*pte & PTE_U)) {
            release(&p->lock);
            return -1;
        }

        // 如果页面被访问过，设置对应的位
        if (*pte & PTE_A) {
            *pte &= ~PTE_A;  // 清除访问标志
            buf |= (1UL << i);  // 在 buf 中设置相应的位
        }
    }

    release(&p->lock);  // 解锁进程

    // 将结果拷贝到用户空间
    if (copyout(p->pagetable, st, (char *)&buf, (len + 7) / 8) < 0) {
        return -1;
    }

    return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}



