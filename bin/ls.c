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

#define COLS 80

char show_hidden = 0;
char long_format = 0;
char show_inodes = 0;
char single_col = 0;
char multiple = 0;
char first = 1;
char last_was_dir = 0;

enum sort_type { SORT_OFF, SORT_NAME, SORT_SIZE, SORT_TIME };
enum sort_type sort_by = SORT_NAME;
char reverse_sort = 0;

struct dirinfo {
  struct dirent dir;
  struct stat st;
  int name_len;
};

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

int compare_dirinfo(const void *a, const void *b) {
  struct dirinfo *dirinfo_a = *(struct dirinfo **)a;
  struct dirinfo *dirinfo_b = *(struct dirinfo **)b;
  int ret = 0;
  if (sort_by == SORT_SIZE) {
    ret = reverse_sort ? (dirinfo_a->st.st_size - dirinfo_b->st.st_size)
                       : (dirinfo_b->st.st_size - dirinfo_a->st.st_size);
  } else if (sort_by == SORT_TIME) {
    ret = reverse_sort ? (dirinfo_a->st.st_time - dirinfo_b->st.st_time)
                       : (dirinfo_b->st.st_time - dirinfo_a->st.st_time);
  }

  if (ret == 0 || sort_by == SORT_NAME) {
    ret = (reverse_sort) ? strcmp(dirinfo_b->dir.d_name, dirinfo_a->dir.d_name)
                         : strcmp(dirinfo_a->dir.d_name, dirinfo_b->dir.d_name);
  }
  return ret;
}

void read_dir(int fd, struct dirinfo *entries[], int *entries_count) {
  struct dirent dir;
  while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
    if (dir.d_ino != 0 && (dir.d_name[0] != '.' || show_hidden)) {
      entries[*entries_count] = malloc(sizeof(struct dirinfo));
      memcpy(&entries[*entries_count]->dir, &dir, sizeof(struct dirent));
      // int name_len = strlen(entries[*entries_count]->dir.d_name);
      // entries[*entries_count]->name_len = name_len;
      // if (name_len > max_name_len) {
      //   max_name_len = name_len;
      // }
      (*entries_count)++;
    }
  }
}

int get_max_name_len(struct dirinfo *entries[], int entries_count) {
  int max_name_len = 0;
  for (int i = 0; i < entries_count; i++) {
    entries[i]->name_len = strlen(entries[i]->dir.d_name);
    ;

    if (entries[i]->name_len > max_name_len) {
      max_name_len = entries[i]->name_len;
    }
  }
  return max_name_len;
}

void stat_entries(struct dirinfo *entries[], int entries_count, const char *dir_name) {
  for (int i = 0; i < entries_count; i++) {
    char fullname[PATH_MAX];
    snprintf(fullname, PATH_MAX, "%s/%s", dir_name, entries[i]->dir.d_name);
    if (stat(fullname, &entries[i]->st) == -1) {
      fprintf(stderr, "Failed to stat %s\n", fullname);
    }
  }
}

void print_entries(struct dirinfo *entries[], int entries_count, int max_name_len, const char *parent) {
  int columns = COLS / (max_name_len + 2 + (show_inodes ? 4 : 0));
  int col = 0;
  for (int i = 0; i < entries_count; i++) {
    if (long_format) {
      print_long(parent, entries[i]->dir.d_name, entries[i]->dir.d_ino);
    } else {
      if (!single_col) {
        if (col > 0 && col >= columns) {
          printf("\n");
          col = 0;
        }
      }
      if (show_inodes) {
        printf("%3d ", entries[i]->dir.d_ino);
      }
      printf("%s  ", entries[i]->dir.d_name);
      if (single_col) {
        printf("\n");
      } else {
        for (int s = 0; s < (max_name_len - entries[i]->name_len); s++) {
          printf(" ");
        }
      }
      col++;
    }
  }
}

int ls(const char *name) {
  struct stat st;
  if (stat(name, &st) == -1) {
    perror(name);
    return EXIT_FAILURE;
  }

  if ((st.st_mode & S_IFMT) == S_IFDIR) {
    if (!first)
      printf("\n");

    first = 0;
    last_was_dir = 1;

    int fd = open(name, O_RDONLY);
    if (fd == -1) {
      perror(name);
      return EXIT_FAILURE;
    }

    if (multiple)
      printf("%s:\n", name);

    int entries_count = 0;
    struct dirinfo *entries[256];

    read_dir(fd, entries, &entries_count);
    close(fd);
    int max_name_len = get_max_name_len(entries, entries_count);
    if (sort_by == SORT_SIZE || sort_by == SORT_TIME) {
      stat_entries(entries, entries_count, name);
    }

    if (sort_by != SORT_OFF) {
      qsort(entries, entries_count, sizeof(struct dirinfo *), compare_dirinfo);
    }

    print_entries(entries, entries_count, max_name_len, name);

    for (int i = 0; i < entries_count; i++) {
      free(entries[i]);
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
  return 0;
}

void usage() {
  fprintf(stderr, "usage: ls [-lai1rSU] [DIR...]\n");
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
      case 'S':
        sort_by = SORT_SIZE;
        break;
      case 'U':
        sort_by = SORT_OFF;
        break;
      case 'r':
        reverse_sort = 1;
        break;
      case 't':
        sort_by = SORT_TIME;
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
      ret = EXIT_FAILURE;
    }
    if (!long_format && !single_col && ret == EXIT_SUCCESS)
      printf("\n");

    ++arg;
  }
  return ret;
}
