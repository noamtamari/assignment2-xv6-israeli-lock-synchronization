#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
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
sys_lcg_srand(void)
{
  int seed;

  argint(0, &seed);
  lcg_srand((uint)seed);
  return 0;
}

uint64
sys_lcg_rand(void)
{
  return lcg_rand();
}

uint64
sys_setgid(void)
{
  int gid;
  struct proc *p = myproc();

  argint(0, &gid);
  acquire(&p->lock);
  p->gid = gid;
  release(&p->lock);
  return 0;
}

uint64
sys_getgid(void)
{
  int gid;
  struct proc *p = myproc();

  acquire(&p->lock);
  gid = p->gid;
  release(&p->lock);
  return gid;
}

uint64
sys_israeli_create(void)
{
  int favoritism;

  argint(0, &favoritism);
  return israeli_create(favoritism);
}

uint64
sys_israeli_acquire(void)
{
  int lock_id;

  argint(0, &lock_id);
  return israeli_acquire(lock_id);
}

uint64
sys_israeli_release(void)
{
  int lock_id;

  argint(0, &lock_id);
  return israeli_release(lock_id);
}

uint64
sys_israeli_destroy(void)
{
  int lock_id;

  argint(0, &lock_id);
  return israeli_destroy(lock_id);
}

uint64
sys_relay_init(void)
{
  int teams;
  int target;

  argint(0, &teams);
  argint(1, &target);
  return relay_init(teams, target);
}

uint64
sys_relay_score_inc(void)
{
  int team;

  argint(0, &team);
  return relay_score_inc(team);
}

uint64
sys_relay_winner(void)
{
  return relay_winner();
}

uint64
sys_relay_get_scores(void)
{
  uint64 dst;
  int max;

  argaddr(0, &dst);
  argint(1, &max);
  return relay_get_scores(dst, max);
}
