#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEFAULT_FAVORITISM 50
#define DEFAULT_TEAMS 3
#define DEFAULT_RUNNERS 5
#define DEFAULT_TARGET 30
#define MAX_TEAMS 16

// Fisher-Yates shuffle for team assignments
static void
shuffle_teams(int *arr, int n, int seed) {
  lcg_srand(seed);
  for (int i = n - 1; i > 0; i--) {
    int j = lcg_rand() % (i + 1);
    int tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  }
}

static void
usage(void)
{
  printf("Usage: relay_race [favoritism] [teams] [runners] [target]\n");
}

int
/*
 * Run relay race simulation.
 * Usage: relay_race [favoritism] [teams] [runners] [target]
 * - No args: uses defaults (favoritism=50, teams=3, runners=5, target=30).
 * - With args: provide 1 to 4 integers in order; each later value overrides
 *   its default. All values must be positive, and favoritism is 0-100.
 */
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
  int team_assignments[total];
  // Fill team_assignments with balanced teams
  for (int r = 0, idx = 0; r < runners; r++) {
    for (int t = 0; t < teams; t++, idx++) {
      team_assignments[idx] = t;
    }
  }
  // Shuffle the team assignments
  shuffle_teams(team_assignments, total, favoritism);
  for(int i = 0; i < total; i++){
    int pid = fork();

    if(pid < 0){
      printf("relay_race: fork failed\n");
      israeli_destroy(lock_id);
      exit(1);
    }

    if(pid == 0){
      int team = team_assignments[i];
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
