#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <time.h>
#include <unistd.h>

char show_hidden;
char long_format;
char show_inodes;

const char *mname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int ls(const char *name) {
  int fd = open(name, O_RDONLY);
  if (fd == -1)
    return EXIT_FAILURE;

  struct stat st;
  if (fstat(fd, &st) == -1)
    return EXIT_FAILURE;

  if ((st.st_mode & S_IFMT) == S_IFDIR) {
    struct dirent dir;
    while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
      if (dir.d_ino != 0 && (dir.d_name[0] != '.' || show_hidden)) {
        if (long_format) {
          char dirent_name[PATH_MAX];
          char *p = dirent_name;
          strncpy(p, name, PATH_MAX);
          p += strlen(name);
          *p++ = '/';
          strncpy(p, dir.d_name, PATH_MAX);

          struct stat st;
          stat(dirent_name, &st);

          char typ = '-';
          if ((st.st_mode & S_IFMT) == S_IFDIR)
            typ = 'd';
          if ((st.st_mode & S_IFMT) == S_IFCHR)
            typ = 'c';
          if ((st.st_mode & S_IFMT) == S_IFBLK)
            typ = 'b';

          char rusr = (st.st_mode & S_IRUSR) ? 'r' : '-';
          char wusr = (st.st_mode & S_IWUSR) ? 'w' : '-';
          char xusr = (st.st_mode & S_IXUSR) ? 'x' : '-';
          char rgrp = (st.st_mode & S_IRGRP) ? 'r' : '-';
          char wgrp = (st.st_mode & S_IWGRP) ? 'w' : '-';
          char xgrp = (st.st_mode & S_IXGRP) ? 'x' : '-';
          char roth = (st.st_mode & S_IROTH) ? 'r' : '-';
          char woth = (st.st_mode & S_IWOTH) ? 'w' : '-';
          char xoth = (st.st_mode & S_IXOTH) ? 'x' : '-';

          if (show_inodes) {
            printf("%3d ", dir.d_ino);
          }
          printf("%c", typ);
          printf("%c%c%c", rusr, wusr, xusr);
          printf("%c%c%c", rgrp, wgrp, xgrp);
          printf("%c%c%c", roth, woth, xoth);

          printf(" %d", st.st_nlink);

          struct passwd *pw = getpwuid(st.st_uid);
          if (pw != NULL) {
            printf(" %6s", pw->pw_name);
          } else {
            printf(" %6d", st.st_uid);
          }

          struct group *gr = getgrgid(st.st_gid);
          if (gr != NULL) {
            printf(" %6s", gr->gr_name);
          } else {
            printf(" %6d", st.st_gid);
          }

          printf(" %6d", st.st_size);

          struct tm *tm = gmtime(&st.st_time);
          printf(" %s %3d %02d:%02d", mname[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min);
          printf(" %s\n", dir.d_name);
        } else {
          if (show_inodes) {
            printf("%3d ", dir.d_ino);
          }
          printf("%s ", dir.d_name);
        }
      }
    }
  } else {
    printf("%s ", name);
  }
  close(fd);
  return 0;
}

int main(int argc, char **argv) {
  show_hidden = 0; // TODO: Default to 1 if uid == 0
  long_format = 0;

  int arg = 1;
  int o = 1;
  while (arg < argc && argv[arg][0] == '-') {
    switch (argv[arg][o]) {
    case 'a':
      show_hidden = 1;
      break;
    case 'l':
      long_format = 1;
      break;
    case 'i':
      show_inodes = 1;
      break;
    default:
      fprintf(stderr, "%s: Unknown option -%c\n", argv[0], argv[arg][o]);
      exit(EXIT_FAILURE);
    }

    ++o;
    if (argv[arg][o] == 0) {
      o = 1;
      ++arg;
    }
  }

  int ret = EXIT_SUCCESS;
  if (arg == argc) {
    ret = ls(".");
    if (!long_format && ret == EXIT_SUCCESS)
      printf("\n");
  }

  while (arg < argc) {
    if (ls(argv[arg]) != 0) {
      fprintf(stderr, "%s: Failed to open %s\n", argv[0], argv[arg]);
      ret = EXIT_FAILURE;
    }
    if (!long_format && ret == EXIT_SUCCESS)
      printf("\n");
    ++arg;
  }
  return ret;
}
