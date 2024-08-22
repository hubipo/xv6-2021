#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"
extern int getnproc(void);      // 声明外部函数 getnproc
extern int getfreemem(void);    // 声明外部函数 getfreemem


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
uint64
sys_trace(void)
{
  int mask;
  argint(0,&mask);
  myproc()->trace_mask=mask;
  return 0; // 返回 0 表示系统调用执行成功
}

uint64 sys_sysinfo(void)
{
  struct proc *p = myproc();    // 获取当前进程的指针
  struct sysinfo st;            // 用于保存系统信息的结构体
  uint64 addr;                  // 用户传入的 sysinfo 结构体指针

  // 获取系统的空闲内存和正在运行的进程数
  st.freemem = getfreemem();
  st.nproc = getnproc();

  // 从用户空间获取 sysinfo 结构体的地址
  if (argaddr(0, &addr) < 0)
    return -1;

  // 将 sysinfo 结构体数据从内核复制到用户空间
  if (copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
    return -1;

  return 0;
}