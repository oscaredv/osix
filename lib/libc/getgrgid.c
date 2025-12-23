#include <fcntl.h>
#include <grp.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

static char *grp_cache_data = NULL;
static size_t grp_cache_size = 0;
static struct group *grp_cache = NULL;

int parse_group() {
  int fd = open("/etc/group", O_RDONLY);
  struct stat st;
  if (fstat(fd, &st) == -1 || st.st_size == 0) {
    close(fd);
    return -1;
  }

  grp_cache_data = malloc(st.st_size + 1);
  if (grp_cache_data == NULL) {
    // TODO: ENOMEM
    close(fd);
    return -1;
  }

  // Load password file
  if (read(fd, grp_cache_data, st.st_size) != st.st_size) {
    close(fd);
    return -1;
  }
  grp_cache_data[st.st_size] = 0;
  close(fd);

  // Count entries
  char *p = grp_cache_data;
  grp_cache_size = 0;
  while (*p != 0) {
    if (*p++ == '\n') // Count newlines
      ++grp_cache_size;
    else if (*p == 0) // Count entry even if newline is missing
      ++grp_cache_size;
  }

  grp_cache = malloc(grp_cache_size * sizeof(struct group));
  if (grp_cache == NULL) {
    free(grp_cache_data);
    grp_cache_data = NULL;
    return -1; // TODO: ENOMEM
  }

  p = grp_cache_data;
  for (size_t e = 0; e < grp_cache_size; e++) {
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

    grp_cache[e].gr_name = fields[0];
    grp_cache[e].gr_passwd = fields[1];
    grp_cache[e].gr_gid = atoi(fields[2]);
    for (int g = 0; g < NGROUPMEMB; g++) {
      grp_cache[e].gr_mem[g] = NULL;
    }

    char *p = fields[3];
    int g = 0;
    while (*p != 0 && g < NGROUPMEMB) {
      grp_cache[e].gr_mem[g++] = p;
      while (*p != 0 && *p != ',')
        ++p;

      if (*p == ',')
        *p++ = 0;
    }
  }

  return 0;
}

struct group *getgrgid(short gid) {
  if (grp_cache == NULL && parse_group() == -1)
    return NULL;

  for (size_t i = 0; i < grp_cache_size; i++) {
    if (grp_cache[i].gr_gid == gid) {
      return &grp_cache[i];
    }
  }

  return NULL;
}

int initgroups(const char *name, gid_t basegid) {
  gid_t groups[NGROUPS];
  int ngroups = 0;
  if (getgrouplist(name, basegid, groups, &ngroups)) {
    return -1;
  }

  return setgroups(ngroups, groups);
}

int getgrouplist(const char *name, gid_t basegid, gid_t *groups, int *ngroups) {
  if (grp_cache == NULL && parse_group() == -1)
    return -1;

  *ngroups = 0;
  groups[(*ngroups)++] = basegid;
  for (unsigned g = 0; g < grp_cache_size; g++) {
    for (int m = 0; m < NGROUPMEMB && grp_cache[g].gr_mem[m] != NULL; m++) {
      if (!strcmp(name, grp_cache[g].gr_mem[m])) {
        groups[(*ngroups)++] = grp_cache[g].gr_gid;
        if (*ngroups >= NGROUPS)
          return 0;
      }
    }
  }
  return 0;
}
