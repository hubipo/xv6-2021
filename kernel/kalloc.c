// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char lockname[8];
} kmems[NCPU];

void
kinit()
{
   // init the kmem array - lab8-1
  int i;
  for (i = 0; i < NCPU; ++i) {
    snprintf(kmems[i].lockname, 8, "kmem_%d", i);    // the name of the lock
    initlock(&kmems[i].lock, kmems[i].lockname);
  }
//  initlock(&kmem.lock, "kmem");   // lab8-1
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int c;    // cpuid - lab8-1

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // get the current core number - lab8-1
  push_off();
  c = cpuid();
  pop_off();
  // free the page to the current cpu's freelist - lab8-1
  acquire(&kmems[c].lock);
  r->next = kmems[c].freelist;
  kmems[c].freelist = r;
  release(&kmems[c].lock);
}

struct run *steal(int cpu_id) {
    int c = cpu_id;
    struct run *fast, *slow, *head;

    // 确认当前 CPU ID 未发生变化，否则触发 panic
    if (cpu_id != cpuid()) {
        panic("steal");
    }

    // 遍历其他 CPU 的空闲内存链表，尝试获取可用页
    for (int i = 1; i < NCPU; ++i) {
        c = (c + 1) % NCPU;

        acquire(&kmems[c].lock);

        if (kmems[c].freelist) {
            // 使用快慢指针拆分链表
            slow = head = kmems[c].freelist;
            fast = slow->next;

            while (fast && fast->next) {
                slow = slow->next;
                fast = fast->next->next;
            }

            // 将链表后半部分留给原 CPU，其余部分作为当前 CPU 的空闲页
            kmems[c].freelist = slow->next;
            slow->next = 0; // 断开与后半部分的链接

            release(&kmems[c].lock);
            return head; // 返回当前 CPU 可用的链表头
        }

        release(&kmems[c].lock);
    }

    return 0; // 所有 CPU 均无可用页时返回 NULL
}
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  // lab8-1
  int c;
  push_off();
  c = cpuid();
  pop_off();
  // get the page from the current cpu's freelist
  acquire(&kmems[c].lock);
  r = kmems[c].freelist;
  if(r)
    kmems[c].freelist = r->next;
  release(&kmems[c].lock);
  // steal page - lab8-1
  // 若当前CPU空闲物理页为空,且偷取到了物理页
  if(!r && (r = steal(c))) {
    // 加锁修改当前CPU空闲物理页链表
    acquire(&kmems[c].lock);
    kmems[c].freelist = r->next;
    release(&kmems[c].lock);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
