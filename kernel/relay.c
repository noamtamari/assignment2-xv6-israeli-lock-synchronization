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

// Initialize the relay state and scores.
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

// Set up a new relay race with the given number of teams and target score.
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

// Increment the score for the given team.
// Returns new score or -1 on error.
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
  // no team has won yet and this team has reached the target score
  if(relay.winner < 0 && score >= relay.target){
    relay.winner = team;
  }
  release(&relay.lock);
  return score;
}

// Return the index of the winning team, or -1 if no winner yet.
int
relay_winner(void)
{
  int winner;

  acquire(&relay.lock);
  winner = relay.winner;
  release(&relay.lock);
  return winner;
}

// Copy the current scores to a user buffer.
// dst: user-space address to copy scores to
// max: maximum number of scores to copy (user buffer size)
// Returns the number of scores copied, or -1 on error.
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
  // Defensive: Only copy as many scores as the user-requested maximum (max),
  // since the kernel cannot know the actual buffer size. This prevents overflow
  // if the user provides a buffer smaller than the number of teams.
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
