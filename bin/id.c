#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage() {
  fprintf(stderr, "usage: id [OPTIONS]\n");
  fprintf(stderr, "  -u        Show user ID\n");
  fprintf(stderr, "  -g        Show group ID\n");
  fprintf(stderr, "  -h        show this help message\n");
  exit(EXIT_SUCCESS);
}

void bad_option(const char *option) {
  fprintf(stderr, "id: Unknown option: %s, use -h for help!\n", option);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  uid_t uid = getuid();
  gid_t gid = getgid();

  int arg = 1;
  while (arg < argc) {
    if (argv[arg][0] != '-') {
      bad_option(argv[arg]);
    }

    switch (argv[arg][1]) {
    case 'h':
      usage();
      break;
    case 'u':
      printf("%d\n", uid);
      exit(EXIT_SUCCESS);
      break;
    case 'g':
      printf("%d\n", gid);
      exit(EXIT_SUCCESS);
      break;
    default:
      bad_option(argv[arg]);
    }
    ++arg;
  }

  printf("uid=%d", uid);
  struct passwd *pw = getpwuid(uid);
  if (pw) {
    printf("(%s)", pw->pw_name);
  }

  printf(" gid=%d", gid);
  struct group *gr = getgrgid(gid);
  if (gr) {
    printf("(%s)", gr->gr_name);
  }

  int ngroups = getgroups(0, NULL);
  if (ngroups > 0) {
    printf(" groups=");
    gid_t *groups = malloc(ngroups * sizeof(gid_t));
    getgroups(ngroups, groups);
    for (int i = 0; i < ngroups; i++) {
      gid_t g = groups[i];
      struct group *gr = getgrgid(g);
      if (i > 0)
        printf(",");
      printf("%d", g);
      if (gr) {
        printf("(%s)", gr->gr_name);
      }
    }
    free(groups);
  }
  printf("\n");

  return 0;
}
