#include <fcntl.h>
#include <grp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *group_file = NULL;
static size_t group_count = 0;
static struct group *groups = NULL;

int parse_group() {
  int fd = open("/etc/group", O_RDONLY);
  struct stat st;
  if (fstat(fd, &st) == -1 || st.st_size == 0) {
    close(fd);
    return -1;
  }

  group_file = malloc(st.st_size + 1);
  if (group_file == NULL) {
    // TODO: ENOMEM
    close(fd);
    return -1;
  }

  // Load password file
  if (read(fd, group_file, st.st_size) != st.st_size) {
    close(fd);
    return -1;
  }
  group_file[st.st_size] = 0;
  close(fd);

  // Count entries
  char *p = group_file;
  group_count = 0;
  while (*p != 0) {
    if (*p++ == '\n') // Count newlines
      ++group_count;
    else if (*p == 0) // Count entry even if newline is missing
      ++group_count;
  }

  groups = malloc(group_count * sizeof(struct group));
  if (groups == NULL) {
    free(group_file);
    group_file = NULL;
    return -1; // TODO: ENOMEM
  }

  p = group_file;
  for (size_t e = 0; e < group_count; e++) {
    char *fields[] = {NULL, NULL, NULL, NULL};
    for (int f = 0; f < 4; f++) {
      fields[f] = p;
      while (*p != ':' && *p != '\n' && *p != 0)
        ++p;

      char op = *p;
      *p++ = 0;
      if (op == '\n' || op == 0) {
        break;
      }
    }

    groups[e].gr_name = fields[0];
    groups[e].gr_passwd = fields[1];
    groups[e].gr_gid = atoi(fields[2]);
    groups[e].gr_mem = NULL;
  }

  return 0;
}

struct group *getgrgid(short gid) {
  if (groups == NULL && parse_group() == -1)
    return NULL;

  for (size_t i = 0; i < group_count; i++) {
    if (groups[i].gr_gid == gid) {
      return &groups[i];
    }
  }

  return NULL;
}
