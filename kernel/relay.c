#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_TEAMS 16

struct relay_state {
  struct spinlock lock;
  int teams;
  int target;
  int winner;
  int scores[MAX_TEAMS];
};

static struct relay_state relay;

void
relay_system_init(void)
{
  int i;

  initlock(&relay.lock, "relay");
  relay.teams = 0;
  relay.target = 0;
  relay.winner = -1;
  for(i = 0; i < MAX_TEAMS; i++){
    relay.scores[i] = 0;
  }
}

int
relay_init(int teams, int target)
{
  int i;

  if(teams <= 0 || teams > MAX_TEAMS || target <= 0){
    return -1;
  }

  acquire(&relay.lock);
  relay.teams = teams;
  relay.target = target;
  relay.winner = -1;
  for(i = 0; i < MAX_TEAMS; i++){
    relay.scores[i] = 0;
  }
  release(&relay.lock);
  return 0;
}

int
relay_score_inc(int team)
{
  int score;

  acquire(&relay.lock);
  if(relay.teams <= 0 || team < 0 || team >= relay.teams){
    release(&relay.lock);
    return -1;
  }

  score = ++relay.scores[team];
  if(relay.winner < 0 && score >= relay.target){
    relay.winner = team;
  }
  release(&relay.lock);
  return score;
}

int
relay_winner(void)
{
  int winner;

  acquire(&relay.lock);
  winner = relay.winner;
  release(&relay.lock);
  return winner;
}

int
relay_get_scores(uint64 dst, int max)
{
  struct proc *p = myproc();
  int n;
  int ret;

  acquire(&relay.lock);
  if(relay.teams <= 0 || max <= 0){
    release(&relay.lock);
    return -1;
  }

  n = relay.teams;
  if(n > max){
    n = max;
  }
  ret = copyout(p->pagetable, dst, (char *)relay.scores, n * sizeof(int));
  release(&relay.lock);

  if(ret < 0){
    return -1;
  }
  return n;
}
