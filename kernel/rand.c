#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

static uint lcg_state;
static struct spinlock lcg_lock;

void
randinit(void)
{
  initlock(&lcg_lock, "lcg");
  lcg_state = 0;
}

void
lcg_srand(uint seed)
{
  acquire(&lcg_lock);
  lcg_state = seed;
  release(&lcg_lock);
}

uint
lcg_rand(void)
{
  uint val;

  acquire(&lcg_lock);
  lcg_state = lcg_state * 1664525U + 1013904223U;
  val = lcg_state;
  release(&lcg_lock);

  return val;
}
