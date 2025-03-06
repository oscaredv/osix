#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc > 1) {
    fprintf(stderr, "%s: No arguments expected\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char pwd_head[PATH_MAX];
  char *pwd = pwd_head;
  pwd += PATH_MAX;
  *--pwd = 0;

  char dotdot[PATH_MAX];
  memset(dotdot, 0, PATH_MAX);
  char *dotdot_tail = dotdot;

  struct stat st;
  if (stat("/", &st) == -1)
    exit(EXIT_FAILURE);
  unsigned int root_inode = st.st_ino;

  if (stat(".", &st) == -1)
    exit(EXIT_FAILURE);
  unsigned int inode_no = st.st_ino;

  while (root_inode != inode_no) {
    *dotdot_tail++ = '.';
    *dotdot_tail++ = '.';
    *dotdot_tail++ = '/';

    int fd = open(dotdot, O_RDONLY);
    if (fd == -1)
      exit(EXIT_FAILURE);

    struct dirent dir;
    while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
      if (dir.d_ino != 0 && dir.d_ino == inode_no) {
        int len = strlen(dir.d_name);
        for (int i = len - 1; i >= 0; i--) {
          *--pwd = dir.d_name[i];
        }
        *--pwd = '/';
        break;
      }
    }

    // if (stat(dotdot, &st) == -1)
    if (fstat(fd, &st) == -1)
      exit(EXIT_FAILURE);

    close(fd);
    inode_no = st.st_ino;
  }

  if (*pwd != '/')
    *--pwd = '/';

  printf("%s\n", pwd);
  return 0;
}
