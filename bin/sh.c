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
#define MAX_PIPES 16

struct Command {
  char *argv[MAX_ARGS];
  char quoted[MAX_ARGS];
  char *input;
  char *output;
  int append;
};

struct Pipeline {
  struct Command cmds[MAX_CMDS];
  int cmds_count;
};

struct Pipeline pipelines[MAX_PIPES];
int pipelines_count;

char prompt[PROMPT_SIZE] = "$ ";

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

  for (int i = 0; argv[0][i]; i++) {
    // Set variable
    if (argv[0][i] == '=') {
      argv[0][i] = 0;
      char *varname = argv[0];
      char *value = &argv[0][i + 1];
      setenv(varname, value, 1);
      return 1;
    }
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

              argv[*argc] = strdup(fullname);
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

void glob(struct Pipeline *pipeline) {
  for (int c = 0; c < pipeline->cmds_count; c++) {
    int a = 1;
    while (pipeline->cmds[c].argv[a] != NULL) {
      int old_argc = a;
      if (!pipeline->cmds[c].quoted[a]) {
        char *arg = pipeline->cmds[c].argv[a];
        explode_pattern("", arg, &a, pipeline->cmds[c].argv);
      }
      if (a == old_argc)
        a++;
    }
  }
}

void exec_pipeline(struct Pipeline *pipeline) {
  int pipes[2 * (pipeline->cmds_count - 1)];

  for (int i = 0; i < pipeline->cmds_count - 1; i++) {
    pipe(pipes + i * 2);
  }

  for (int i = 0; i < pipeline->cmds_count; i++) {
    if (internal_cmd(pipeline->cmds[i].argv)) {
      continue;
    }

    pid_t pid = fork();
    if (pid == 0) {
      // Connect output to input
      if (i > 0) {
        dup2(pipes[(i - 1) * 2], STDIN_FILENO);
      }
      if (i < pipeline->cmds_count - 1) {
        dup2(pipes[i * 2 + 1], STDOUT_FILENO);
      }

      // Redirections
      if (pipeline->cmds[i].input) {
        int fd = open(pipeline->cmds[i].input, O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (pipeline->cmds[i].output) {
        int flags = O_WRONLY | O_CREAT | (pipeline->cmds[i].append ? O_APPEND : O_TRUNC);
        int fd = open(pipeline->cmds[i].output, flags, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }

      // close all pipes
      for (int j = 0; j < 2 * (pipeline->cmds_count - 1); j++) {
        close(pipes[j]);
      }

      // Execute command
      int ret = execvp(pipeline->cmds[i].argv[0], pipeline->cmds[i].argv);
      perror(pipeline->cmds[i].argv[0]);
      exit(ret);
    }
  }

  // parent closes pipes
  for (int i = 0; i < 2 * (pipeline->cmds_count - 1); i++) {
    close(pipes[i]);
  }
}

char *expand_variable(char *arg) {
  // TODO: Handle ${VAR} syntax and scan entire word for variables, not just the first character
  // TODO: Handle escaping of $ with \$
  // TODO: Handle variables in redirection targets
  // TODO: Handle ~ expansion to $HOME
  if (arg[0] != '$')
    return arg;

  char *var = getenv(&arg[1]);
  if (var) {
    free(arg);
    return strdup(var);
  }
  free(arg);
  return strdup("");
}

void expand_variables(struct Pipeline *pipeline) {
  for (int c = 0; c < pipeline->cmds_count; c++) {
    for (int a = 0; pipeline->cmds[c].argv[a] != NULL; a++) {
      char *arg = pipeline->cmds[c].argv[a];
      pipeline->cmds[c].argv[a] = expand_variable(arg);
    }
  }
}

char *parse_redir(struct Command *cmd, char *p) {
  char input = *p++ == '<' ? 1 : 0;
  if (*p == '>') {
    input = 0;
    cmd->append = 1;
    p++;
  }

  while (*p == ' ')
    p++;

  char *target = p;
  while (*p != ' ' && *p != 0 && *p != '<' && *p != '>' && *p != '|' && *p != ';')
    ++p;

  char old = *p;
  *p = 0;
  target = expand_variable(strdup(target));
  if (input) {
    cmd->input = target;
  } else {
    cmd->output = target;
  }

  *p = old;
  if (*p == '<' || *p == '>') {
    p = parse_redir(cmd, p);
  }

  return p;
}

void sighandler(int signum) {
  (void)signum;
  printf("^C\n");
}

void copy_token(char *dst, const char *src, int len) {
  int j = 0;
  char quote = 0;
  for (int i = 0; i < len; i++) {
    if (src[i] == '\"') {
      quote = !quote;
    }

    if (src[i] == '\\' && i + 1 < len && !quote && (src[i + 1] == ' ' || src[i + 1] == '\\' || src[i + 1] == '\"')) {
      dst[j++] = src[++i];
    } else if (src[i] == '\"') {
      // skip quotes
    } else {
      dst[j++] = src[i];
    }
  }
  dst[j] = 0;
}

void reset_pipelines() {
  for (int p = 0; p < MAX_PIPES; p++) {
    pipelines[p].cmds_count = 0;
    for (int c = 0; c < MAX_CMDS; c++) {
      for (int a = 0; a < MAX_ARGS; a++) {
        pipelines[p].cmds[c].argv[a] = NULL;
        pipelines[p].cmds[c].quoted[a] = 0;
      }
      pipelines[p].cmds[c].input = NULL;
      pipelines[p].cmds[c].output = NULL;
      pipelines[p].cmds[c].append = 0;
    }
  }
  pipelines_count = 0;
}

void tokenize(char *cmdline) {
  char *p = cmdline;
  char quote = 0;

  int pipe_index = 0;
  int cmd_index = 0;
  int arg_index = 0;

  while (*p != 0) {
    // Skip leading whitespace
    while (isspace(*p))
      p++;

    if (*p == 0)
      break;

    char *start = p;
    // scan string until next unescaped whitespace or end of string
    while ((!isspace(*p) || quote == 1) && *p != 0 && *p != ';' && *p != '|' && *p != '<' && *p != '>') {
      char c = *p++;
      if (c == '\\') {
        ++p;
      } else if (c == '\"') {
        quote = !quote;
      }
    }

    if (p != start) {
      // Copy token to argv
      if (arg_index == 0) {
        if (cmd_index == 0) {
          pipelines_count++;
        }
        pipelines[pipe_index].cmds_count++;
      }
      pipelines[pipe_index].cmds[cmd_index].argv[arg_index] = malloc(p - start + 1);
      copy_token(pipelines[pipe_index].cmds[cmd_index].argv[arg_index], start, p - start);
      pipelines[pipe_index].cmds[cmd_index].quoted[arg_index] = *start == '\"';
      arg_index++;
    }

    if (*p == '|') {
      cmd_index++;
      arg_index = 0;
    } else if (*p == ';') {
      pipelines[pipe_index].cmds_count = ++cmd_index;
      pipe_index++;
      cmd_index = 0;
      arg_index = 0;
    } else if (*p == '<' || *p == '>') {
      p = parse_redir(&pipelines[pipe_index].cmds[cmd_index], p);
    }

    if (*p != 0)
      *p++ = 0;
  }
}

void debug_dump(struct Pipeline *pipeline) {
  printf("Pipeline:\n");
  for (int c = 0; c < pipeline->cmds_count; c++) {
    printf("  Command %d:\n", c);
    for (int a = 0; pipeline->cmds[c].argv[a] != NULL; a++) {
      printf("    argv[%d]: '%s' quoted=%d in=%s out=%s\n", a, pipeline->cmds[c].argv[a], pipeline->cmds[c].quoted[a],
             pipeline->cmds[c].input ? pipeline->cmds[c].input : "NULL",
             pipeline->cmds[c].output ? pipeline->cmds[c].output : "NULL");
    }
  }
}

void wait_for_children() {
  while (wait(NULL) > 0)
    ;
}

void free_pipeline(struct Pipeline *pipeline) {
  for (int c = 0; c < pipeline->cmds_count; c++) {
    for (int a = 0; pipeline->cmds[c].argv[a] != NULL; a++) {
      free(pipeline->cmds[c].argv[a]);
      pipeline->cmds[c].argv[a] = NULL;
    }
  }
}

int main(void) {
  signal(SIGINT, sighandler);

  if (getuid() == 0) {
    strncpy(prompt, "# ", PROMPT_SIZE);
  }

  // TODO: IFS handling: Support for setting and using IFS to control word splitting behavior
  // TODO: Sub shell $(cmd): Execute cmd in a subshell and replace with its output
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
      reset_pipelines();
      tokenize(cmdline);

      for (int p = 0; p < pipelines_count; p++) {
        glob(&pipelines[p]);
        expand_variables(&pipelines[p]);
        // debug_dump(&pipelines[p]);
        exec_pipeline(&pipelines[p]);
        wait_for_children();
        free_pipeline(&pipelines[p]);
      }
    }
  }

  printf("exit\n");
  return 0;
}
