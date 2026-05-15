#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
usage(void)
{
  printf("Usage:\n");
  printf("  ass2test rand [seed] [count]\n");
  printf("\n");
  printf("Subcommands:\n");
  printf("  rand   Task 0: generate LCG sequence\n");
  printf("  lock   Task 1 placeholder\n");
  printf("  relay  Task 2 placeholder\n");
}

static int
parse_int(const char *s, int def)
{
  if(s == 0 || s[0] == '\0'){
    return def;
  }
  return atoi(s);
}

static void
u32_to_dec(uint x, char *buf)
{
  char tmp[11];
  int i = 0;
  int j = 0;

  if(x == 0){
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }

  while(x > 0){
    tmp[i++] = '0' + (x % 10);
    x /= 10;
  }
  while(i > 0){
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';
}

static int
cmd_rand(int argc, char *argv[])
{
  int seed = 1;
  int count = 5;
  int i;

  if(argc >= 1){
    seed = parse_int(argv[0], seed);
  }
  if(argc >= 2){
    count = parse_int(argv[1], count);
  }
  if(count < 0){
    printf("ass2test: count must be non-negative\n");
    return 1;
  }

  lcg_srand((uint)seed);
  for(i = 0; i < count; i++){
    uint val = lcg_rand();
    char buf[11];
    u32_to_dec(val, buf);
    printf("%d: %s\n", i, buf);
  }

  return 0;
}

static int
cmd_lock(void)
{
  printf("ass2test: lock subcommand not implemented yet\n");
  return 1;
}

static int
cmd_relay(void)
{
  printf("ass2test: relay subcommand not implemented yet\n");
  return 1;
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    usage();
    exit(1);
  }

  if(strcmp(argv[1], "rand") == 0){
    exit(cmd_rand(argc - 2, &argv[2]));
  }
  if(strcmp(argv[1], "lock") == 0){
    exit(cmd_lock());
  }
  if(strcmp(argv[1], "relay") == 0){
    exit(cmd_relay());
  }

  usage();
  exit(1);
}
