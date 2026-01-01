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

char show_hidden = 0;
char long_format = 0;
char show_inodes = 0;
char single_col = 0;
char multiple = 0;
char first = 1;
char last_was_dir = 0;

const char *mname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void print_long(const char *base, const char *name, unsigned short ino) {
  char dirent_name[PATH_MAX];
  char *p = dirent_name;
  strncpy(p, base, PATH_MAX);
  p += strlen(base);
  *p++ = '/';
  strncpy(p, name, PATH_MAX);

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
    printf("%3d ", ino);
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
  printf(" %s\n", name);
}

int ls(const char *name) {
  int fd = open(name, O_RDONLY);
  if (fd == -1)
    return EXIT_FAILURE;

  struct stat st;
  if (fstat(fd, &st) == -1)
    return EXIT_FAILURE;

  if ((st.st_mode & S_IFMT) == S_IFDIR) {
    if (!first)
      printf("\n");

    first = 0;
    last_was_dir = 1;

    if (multiple)
      printf("%s:\n", name);

    struct dirent dir;
    while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
      if (dir.d_ino != 0 && (dir.d_name[0] != '.' || show_hidden)) {
        if (long_format) {
          print_long(name, dir.d_name, dir.d_ino);
        } else {
          if (show_inodes) {
            printf("%3d ", dir.d_ino);
          }
          printf("%s ", dir.d_name);
          if (single_col) {
            printf("\n");
          }
        }
      }
    }
  } else {
    if (last_was_dir)
      printf("\n");

    first = 0;
    last_was_dir = 0;

    if (long_format) {
      print_long(".", name, 99);
    } else {
      printf("%s ", name);
      if (single_col) {
        printf("\n");
      }
    }
  }
  close(fd);
  return 0;
}

void usage() {
  fprintf(stderr, "usage: ls [-lai1] [DIR...]\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (!isatty(STDOUT_FILENO)) {
    single_col = 1;
  }

  int arg = 1;
  while (arg < argc && argv[arg][0] == '-') {
    for (int o = 1; argv[arg][o] != '\0'; o++) {
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
      case '1':
        single_col = 1;
        break;
      case 'h':
        usage();
        break;
      default:
        fprintf(stderr, "%s: Unknown option -%c\n", argv[0], argv[arg][o]);
        exit(EXIT_FAILURE);
      }
    }
    ++arg;
  }

  int ret = EXIT_SUCCESS;
  if (arg == argc) {
    ret = ls(".");
    if (!long_format && !single_col && ret == EXIT_SUCCESS)
      printf("\n");
  }

  multiple = (argc - arg) > 1;
  while (arg < argc) {
    if (ls(argv[arg]) != 0) {
      fprintf(stderr, "%s: Failed to open %s\n", argv[0], argv[arg]);
      ret = EXIT_FAILURE;
    }
    if (!long_format && !single_col && ret == EXIT_SUCCESS)
      printf("\n");

    ++arg;
  }
  return ret;
}
