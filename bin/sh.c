#include <ctype.h>
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

#define MAX_LINE 1024
#define PROMPT_SIZE 16
#define MAX_ARGS 256
#define MAX_CMDS 16

struct Command {
  char *argv[MAX_ARGS];
  char *input;
  char *output;
  int append;
};
struct Command cmds[MAX_CMDS];

char prompt[PROMPT_SIZE] = "$ ";

char *allocs[MAX_ARGS];
int alloc_count;

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

char *alloc_string(const char *s) {
  int len = strlen(s) + 1;
  char *p = malloc(len);
  strncpy(p, s, len);
  allocs[alloc_count++] = p;
  return p;
}

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

              argv[*argc] = alloc_string(fullname);
              (*argc)++;
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

void debug_pipeline(struct Command *cmds, int n) {
  for (int i = 0; i < n; i++) {
    printf("  Command %d: input=%s output=%s append=%d\n", i, cmds[i].input, cmds[i].output, cmds[i].append);
    for (int a = 0; a < MAX_ARGS && cmds[i].argv[a] != NULL; a++) {
      printf("    argv[%d]: %s\n", a, cmds[i].argv[a]);
    }
    if (i < n - 1) {
      printf("   |\n");
    }
  }
  printf("\n");
}

void exec_pipeline(struct Command *cmds, int n) {
  int pipes[2 * (n - 1)];

  for (int i = 0; i < n - 1; i++) {
    pipe(pipes + i * 2);
  }

  for (int i = 0; i < n; i++) {
    if (internal_cmd(cmds[i].argv)) {
      continue;
    }

    pid_t pid = fork();
    if (pid == 0) {
      // Connect output to input
      if (i > 0) {
        dup2(pipes[(i - 1) * 2], STDIN_FILENO);
      }
      if (i < n - 1) {
        dup2(pipes[i * 2 + 1], STDOUT_FILENO);
      }

      // Redirections
      if (cmds[i].input) {
        int fd = open(cmds[i].input, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (cmds[i].output) {
        int flags = O_WRONLY | O_CREAT | (cmds[i].append ? O_APPEND : O_TRUNC);
        int fd = open(cmds[i].output, flags, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      // close all pipes
      for (int j = 0; j < 2 * (n - 1); j++) {
        close(pipes[j]);
      }

      char path[strlen(cmds[i].argv[0]) + 6];
      if (*cmds[i].argv[0] == '/' || *cmds[i].argv[0] == '.') {
        snprintf(path, sizeof(path), "%s", cmds[i].argv[0]);
      } else {
        snprintf(path, sizeof(path), "/bin/%s", cmds[i].argv[0]);
      }
      int ret = execv(path, cmds[i].argv);
      perror(cmds[i].argv[0]);
      exit(ret);
    }
  }

  // parent closes pipes
  for (int i = 0; i < 2 * (n - 1); i++) {
    close(pipes[i]);
  }
}

char *resolve_var(char *p) {
  if (p[0] != '$')
    return p;

  char *var = getenv(&p[1]);
  if (var)
    return var;
  return p;
}

char *parse_redir(struct Command *cmd, char *p) {
  int input = 0;

  if (*p == '<')
    input = 1;

  *p++ = 0;

  if (*p == '>') {
    input = 0;
    cmd->append = 1;
    p++;
  }

  while (*p == ' ')
    p++;

  if (input)
    cmd->input = p;
  else
    cmd->output = p;

  while (*p != ' ' && *p != 0 && *p != '<' && *p != '>')
    ++p;

  if (*p == '<' || *p == '>') {
    p = parse_redir(cmd, p);
  } else if (*p != 0) {
    *p++ = 0;
  }

  if (cmd->input)
    cmd->input = resolve_var(cmd->input);
  if (cmd->output)
    cmd->output = resolve_var(cmd->output);

  return p;
}

void parse_cmd(struct Command *cmd, char *cmdline) {
  int argc = 0;
  char *p = cmdline;
  while (*p != 0) {
    while (*p == ' ')
      p++;
    if (*p == 0)
      break;

    if (*p == '<' || *p == '>') {
      p = parse_redir(cmd, p);
      continue;
    }

    cmd->argv[argc] = p;
    char pattern = 0;
    while (*p != ' ' && *p != 0 && *p != '<' && *p != '>') {
      if (*p == '*' || *p == '?')
        pattern = 1;
      ++p;
    }
    if (*p == '<' || *p == '>') {
      p = parse_redir(cmd, p);
    }
    if (*p != 0)
      *p++ = 0;

    // Environment variable expansion
    cmd->argv[argc] = resolve_var(cmd->argv[argc]);

    if (pattern) {
      int old_argc = argc;
      explode_pattern("", cmd->argv[argc], &argc, cmd->argv);
      if (argc == old_argc)
        argc++;
    } else {
      argc++;
    }
  }
  cmd->argv[argc] = NULL;
}

void parse_pipeline(char *cmdline) {
  memset(cmds, 0, sizeof(cmds));
  char *p = cmdline;
  int c = 0;
  while (*p != 0) {
    while (*p == ' ')
      p++;
    char *start = p;
    while (*p != '|' && *p != 0)
      p++;
    if (*p == '|')
      *p++ = 0;

    parse_cmd(&cmds[c++], start);
  }

  exec_pipeline(cmds, c);

  // Free allocated strings
  for (int i = 0; i < alloc_count; i++) {
    free(allocs[i]);
  }
  alloc_count = 0;

  // Wait for children
  for (int i = 0; i < c; i++) {
    wait(NULL);
  }
}

void parse_separator(char *cmdline) {
  char *p = cmdline;
  while (*p != 0) {
    while (*p == ' ')
      p++;
    char *start = p;
    while (*p != ';' && *p != 0)
      p++;
    if (*p == ';')
      *p++ = 0;

    parse_pipeline(start);
  }
}

void sighandler(int signum) {
  (void)signum;
  printf("^C\n");
}

int main(void) {
  signal(SIGINT, sighandler);
  alloc_count = 0;

  if (getuid() == 0) {
    strncpy(prompt, "# ", PROMPT_SIZE);
  }

  char cmdline[MAX_LINE];
  while (1) {
    printf("%s", prompt);
    fflush(stdout);
    int len = read(STDIN_FILENO, cmdline, sizeof(cmdline));

    if (len <= 0 && errno != EINTR) {
      // Exit on EOF or read error
      break;
    } else if (len > 0) {
      cmdline[len - 1] = 0;
      parse_separator(cmdline);
    }
  }

  printf("exit\n");
  return 0;
}
