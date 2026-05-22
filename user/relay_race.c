#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEFAULT_FAVORITISM 50
#define DEFAULT_TEAMS 3
#define DEFAULT_RUNNERS 5
#define DEFAULT_TARGET 30
#define MAX_TEAMS 16

static void
usage(void)
{
  printf("Usage: relay_race [favoritism] [teams] [runners] [target]\n");
}

int
main(int argc, char *argv[])
{
  int favoritism = DEFAULT_FAVORITISM;
  int teams = DEFAULT_TEAMS;
  int runners = DEFAULT_RUNNERS;
  int target = DEFAULT_TARGET;
  int total;
  int lock_id;

  if(argc > 1){
    favoritism = atoi(argv[1]);
  }
  if(argc > 2){
    teams = atoi(argv[2]);
  }
  if(argc > 3){
    runners = atoi(argv[3]);
  }
  if(argc > 4){
    target = atoi(argv[4]);
  }
  if(argc > 5){
    usage();
    exit(1);
  }

  if(favoritism < 0 || favoritism > 100 || teams <= 0 || teams > MAX_TEAMS ||
    runners <= 0 || target <= 0){
    usage();
    exit(1);
  }

  lock_id = israeli_create(favoritism);
  if(lock_id < 0){
    printf("relay_race: failed to create lock\n");
    exit(1);
  }

  if(relay_init(teams, target) < 0){
    printf("relay_race: failed to init relay state\n");
    israeli_destroy(lock_id);
    exit(1);
  }

  total = teams * runners;
  for(int i = 0; i < total; i++){
    int pid = fork();

    if(pid < 0){
      printf("relay_race: fork failed\n");
      israeli_destroy(lock_id);
      exit(1);
    }

    if(pid == 0){
      int team = i % teams;
      setgid(team);
      for(;;){
        if(relay_winner() >= 0){
          break;
        }
        if(israeli_acquire(lock_id) < 0){
          exit(1);
        }
        if(relay_winner() >= 0){
          israeli_release(lock_id);
          break;
        }
        int score = relay_score_inc(team);
        if(score < 0){
          israeli_release(lock_id);
          exit(1);
        }
        printf("Runner %d (team %d) score=%d\n", getpid(), team, score);
        israeli_release(lock_id);
        sleep(1);
      }
      exit(0);
    }
  }

  for(int i = 0; i < total; i++)
    wait(0);

  {
    int scores[MAX_TEAMS];
    int n = relay_get_scores(scores, MAX_TEAMS);
    int winner = relay_winner();

    if(n < 0){
      printf("relay_race: failed to read scores\n");
    } else {
      printf("Final scores:\n");
      for(int i = 0; i < n; i++){
        printf("Team %d: %d\n", i, scores[i]);
      }
      if(winner >= 0){
        printf("Winner: team %d\n", winner);
      }
    }
  }

  israeli_destroy(lock_id);
  exit(0);
}
