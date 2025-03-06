#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int internal_cmd(char *argv[]) {
  if (!strcmp(argv[0], "exit")) {
    exit(EXIT_SUCCESS);
  }

  if (!strcmp(argv[0], "cd")) {
    if (chdir(argv[1]) == -1) {
      fprintf(stderr, "cd: %s: Not a directory\n", argv[1]);
    }
    return 1;
  }

  if (!strcmp(argv[0], "clear")) {
    printf("\e[J");
    printf("\e[H");
    fflush(stdout);
    return 1;
  }

  return 0;
}

int pattern_match(const char *pattern, const char *s) {
  const char *cp = NULL, *mp = NULL;

  while ((*s) && (*pattern != '*')) {
    if ((*pattern != *s) && (*pattern != '?')) {
      return 0;
    }
    ++pattern;
    ++s;
  }

  while (*s) {
    if (*pattern == '*') {
      if (!*++pattern) {
        return 1;
      }
      mp = pattern;
      cp = s + 1;
    } else if ((*pattern == *s) || (*pattern == '?')) {
      pattern++;
      s++;
    } else {
      pattern = mp;
      s = cp++;
    }
  }

  while (*pattern == '*') {
    pattern++;
  }
  return !*pattern;
}

char sstore_buf[1024];
char *sstore;

void explode_pattern(const char *base, const char *pattern, int *argc, char *argv[]) {
  const char *p = pattern;
  if (*p == '/') {
    while (*p == '/')
      ++p;

    explode_pattern("/", p, argc, argv);
  } else {
    while (*p != 0 && *p != '/')
      ++p;

    char last = *p != '/';
    char current_pattern[PATH_MAX];
    strncpy(current_pattern, pattern, p - pattern);
    current_pattern[p - pattern] = 0;

    while (*p == '/')
      ++p;

    int fd = -1;
    if (*base == 0) {
      fd = open(".", O_RDONLY);
    } else {
      fd = open(base, O_RDONLY);
    }

    if (fd != -1) {
      struct dirent dir;
      while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
        if (dir.d_ino != 0 && dir.d_name[0] != '.') {
          if (pattern_match(current_pattern, dir.d_name)) {
            if (last) {
              char fullname[strlen(base) + 1 + strlen(dir.d_name) + 1];
              if (*base == 0) {
                snprintf(fullname, sizeof(fullname), "%s", dir.d_name);
              } else {
                snprintf(fullname, sizeof(fullname), "%s/%s", base, dir.d_name);
              }

              int de_len = strlen(fullname) + 1;
              argv[(*argc)++] = strncpy(sstore, fullname, de_len);
              sstore += de_len;

            } else {
              char next_base[strlen(base) + 1 + strlen(dir.d_name) + 1];
              if (base[0] == 0) {
                snprintf(next_base, sizeof(next_base), "%s", dir.d_name);
              } else if (base[0] == '/' && base[1] == 0) {
                snprintf(next_base, sizeof(next_base), "/%s", dir.d_name);
              } else {
                snprintf(next_base, sizeof(next_base), "%s/%s", base, dir.d_name);
              }

              explode_pattern(next_base, p, argc, argv);
            }
          }
        }
      }
      close(fd);
    }
  }
}

void exec_cmd(char *cmdline) {
  if (*cmdline == 0)
    return;

  sstore = sstore_buf; // Reset string store
  char *argv[256];
  memset(argv, 0, sizeof(argv));

  int argc = 0;
  char *p = cmdline;
  while (*p != 0) {
    while (*p == ' ')
      p++;
    if (*p == 0)
      break;

    argv[argc] = p;
    char pattern = 0;
    while (*p != ' ' && *p != 0 && *p != '\n') {
      if (*p == '*' || *p == '?')
        pattern = 1;
      ++p;
    }
    if (*p == ' ')
      *p++ = 0;

    // Environment variable expansion
    if (argv[argc][0] == '$') {
      char *var = getenv(&argv[argc][1]);
      if (var)
        argv[argc] = var;
    }

    if (pattern) {
      int old_argc = argc;
      explode_pattern("", argv[argc], &argc, argv);
      if (argc == old_argc)
        argc++;
    } else {
      argc++;
    }
  }
  argv[argc] = NULL;

  if (!internal_cmd(argv)) {
    char path[strlen(argv[0]) + 6];
    if (*argv[0] == '/' || *argv[0] == '.') {
      snprintf(path, sizeof(path), "%s", argv[0]);
    } else {
      snprintf(path, sizeof(path), "/bin/%s", argv[0]);
    }

    struct stat st;
    if (stat(path, &st) == -1) {
      fprintf(stderr, "%s: Command not found!\n", argv[0]);
      return;
    }

    pid_t pid = fork();
    if (pid == -1) {
      fprintf(stderr, "Fork failed!\n");
    } else if (pid == 0) {
      exit(execv(path, argv));
    } else {
      int exit_code = 0;
      while (wait(&exit_code) == -1)
        ;

      if (exit_code != 0)
        fprintf(stderr, "exit code=%d\n", exit_code);
    }
  }
}

void sighandler(int signum) {
  (void)signum;
  printf("^C\n");
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  signal(SIGINT, sighandler);

  char cmdline[1024];
  while (1) {
    printf("# ");
    fflush(stdout);
    int len = read(STDIN_FILENO, cmdline, sizeof(cmdline));

    if (len == EOF) {
      break;
    } else if (len > 0) {
      cmdline[len - 1] = 0;
      exec_cmd(cmdline);
    }
  }

  printf("exit\n");
  return 0;
}
