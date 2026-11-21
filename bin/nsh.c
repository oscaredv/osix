#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_LINE 1024
#define HISTORY_MAX_SIZE 16
#define PROMPT_MAX_SIZE 16
#define MAX_ARGS 256
#define MAX_CMDS 16
#define MAX_PIPES 16

struct Command {
  char *argv[MAX_ARGS];
  char quoted[MAX_ARGS];
  char *input;
  char *output;
  int append;
  pid_t pid;
  int background;
};

struct Pipeline {
  struct Command cmds[MAX_CMDS];
  int cmds_count;
};

struct Pipeline pipelines[MAX_PIPES];
int pipelines_count;

char prompt[PROMPT_MAX_SIZE] = "$ ";

int interactive = 1;

void set_var(const char *name, const char *value) {
  // TODO: unsetting variables
  // TODO: internal shell variables
  setenv(name, value, 1);
}

void set_var_int(const char *name, int value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", value);
  set_var(name, buf);
}

int internal_cmd(char *argv[]) {
  if (!strcmp(argv[0], "exit")) {
    exit(EXIT_SUCCESS);
  }

  int rc = 0;
  int is_internal = 0;
  if (!strcmp(argv[0], "cd")) {
    is_internal = 1;
    if (chdir(argv[1]) != 0) {
      perror(argv[1]);
      rc = 1;
    }
  } else if (!strcmp(argv[0], "clear")) {
    is_internal = 1;
    printf("\e[J");
    printf("\e[H");
    fflush(stdout);
  }

  for (int i = 0; argv[0][i]; i++) {
    // Set variable
    if (argv[0][i] == '=') {
      is_internal = 1;
      argv[0][i] = 0;
      char *varname = argv[0];
      char *value = &argv[0][i + 1];
      set_var(varname, value);
      break;
    }
  }
  if (is_internal) {
    set_var_int("?", rc);
  }
  return is_internal;
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

    int argc = 0;
    for (int a = 0; pipeline->cmds[i].argv[a] != NULL; a++) {
      argc++;
    }
    if (strcmp("&", pipeline->cmds[i].argv[argc - 1]) == 0) {
      pipeline->cmds[i].argv[argc - 1] = NULL;
      pipeline->cmds[i].background = 1;
    }

    pipeline->cmds[i].pid = fork();
    if (pipeline->cmds[i].pid == 0) {
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

  if (interactive)
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
      pipelines[p].cmds[c].pid = 0;
      pipelines[p].cmds[c].background = 0;
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
  if (!interactive)
    return;
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

void wait_for_children(struct Pipeline *pipeline) {
  int status;
  pid_t pid = 0;
  // Count PIDs we'll wait for
  int wait_for_pids = 0;
  for (int c = 0; c < pipeline->cmds_count; c++) {
    if (pipeline->cmds[c].pid > 0 && pipeline->cmds[c].background == 0)
      wait_for_pids++;
  }

  while (pid >= 0 && wait_for_pids > 0) {
    // Wait for the next child process to finish
    pid = wait(&status);

    for (int c = 0; c < pipeline->cmds_count; c++) {
      // Does the PID match one of the childs we're waiting for?
      if (pipeline->cmds[c].pid == pid && pipeline->cmds[c].background == 0) {
        wait_for_pids--;
        set_var_int("?", status);
        break;
      }
    }
  }
}

void free_pipeline(struct Pipeline *pipeline) {
  for (int c = 0; c < pipeline->cmds_count; c++) {
    for (int a = 0; pipeline->cmds[c].argv[a] != NULL; a++) {
      free(pipeline->cmds[c].argv[a]);
      pipeline->cmds[c].argv[a] = NULL;
    }
  }
}

void truncate_comments(char *cmdline) {
  char quote = 0;
  char last_char = 0;
  char *p = cmdline;
  while (*p != 0) {
    if (*p == '\"' && last_char != '\\') {
      quote = !quote;
    }
    if (!quote && *p == '#' && last_char != '\\') {
      *p = 0;
      break;
    }
    last_char = *p++;
  }
}

void canonical_mode() {
  if (!interactive)
    return;
  struct termios cooked;
  tcgetattr(STDIN_FILENO, &cooked);
  cooked.c_lflag |= ECHO | ICANON;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

void cbreak_mode() {
  if (!interactive)
    return;
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_lflag |= ISIG;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void redraw(const char *prompt, const char *cmdline, int cursor_pos, int cmdline_len) {
  if (!interactive)
    return;
  printf("\r\033[2K%s%s ", prompt, cmdline);
  for (int i = cursor_pos; i <= cmdline_len; i++) {
    printf("\b");
  }
  fflush(stdout);
}

void execute_command(char *cmdline) {
  truncate_comments(cmdline);
  reset_pipelines();
  tokenize(cmdline);

  for (int p = 0; p < pipelines_count; p++) {
    glob(&pipelines[p]);
    expand_variables(&pipelines[p]);
    // debug_dump(&pipelines[p]);
    exec_pipeline(&pipelines[p]);
    wait_for_children(&pipelines[p]);
    free_pipeline(&pipelines[p]);
  }
}

void insert(char *cmdline, char c, int *cursor_pos, int *cmdline_size) {
  for (int t = *cmdline_size; t > *cursor_pos; t--) {
    cmdline[t] = cmdline[t - 1];
  }
  cmdline[*cursor_pos] = c;
  (*cursor_pos)++;
  (*cmdline_size)++;
  cmdline[*cmdline_size] = 0;
}

void delete_word(char *cmdline, int *cursor_pos, int *cmdline_size) {
  while (*cursor_pos > 0 && cmdline[*cursor_pos - 1] == ' ') {
    (*cursor_pos)--;
    (*cmdline_size)--;
  }
  while (*cursor_pos > 0 && cmdline[*cursor_pos - 1] != ' ') {
    (*cursor_pos)--;
    (*cmdline_size)--;
  }
  for (int t = *cursor_pos; t < *cmdline_size; t++) {
    cmdline[t] = cmdline[t + 1];
  }
  cmdline[*cmdline_size] = 0;
}

void delete_char(char *cmdline, int *cursor_pos, int *cmdline_size) {
  if (*cursor_pos > 0) {
    (*cursor_pos)--;
    (*cmdline_size)--;
    for (int t = *cursor_pos; t < *cmdline_size; t++) {
      cmdline[t] = cmdline[t + 1];
    }
    cmdline[*cmdline_size] = 0;
  }
}

void clear_line(char *cmdline, int *cursor_pos, int *cmdline_size) {
  *cursor_pos = 0;
  *cmdline_size = 0;
  cmdline[0] = 0;
}

void history_save(const char *cmdline, const char **history, int *history_index, int *history_size) {
  // Only save non-empty commands that are not duplicates of the last entry
  if (cmdline[0] != 0) {
    if (*history_size == 0 || strcmp(history[*history_size - 1], cmdline) != 0) {
      if (*history_size == HISTORY_MAX_SIZE) {
        free((void *)history[0]);
        for (int j = 1; j < HISTORY_MAX_SIZE; j++) {
          history[j - 1] = history[j];
        }
        (*history_size)--;
      }
      history[(*history_size)++] = strdup(cmdline);
    }
  }
  *history_index = *history_size;
}

void history_copy(char *cmdline, const char **history, int history_index, int history_size, int *cursor_pos,
                  int *cmdline_size) {
  if (history_index == history_size) {
    cmdline[0] = 0;
    *cmdline_size = *cursor_pos = 0;
  } else {
    strncpy(cmdline, history[history_index], MAX_LINE);
    *cmdline_size = *cursor_pos = strlen(cmdline);
  }
}

void delete(char *cmdline, int cursor_pos, int *cmdline_size) {
  for (int t = cursor_pos; t < *cmdline_size - 1; t++) {
    cmdline[t] = cmdline[t + 1];
  }
  (*cmdline_size)--;
  cmdline[*cmdline_size] = 0;
}

int compare(const void *a, const void *b) { return strcmp(*(const char **)a, *(const char **)b); }

void find_matches(const char *dirname, const char *needle, char **matches, int *matches_size) {
  int needle_len = strlen(needle);

  int fd = open(dirname, O_RDONLY);
  if (fd != -1) {
    struct dirent dir;
    while (read(fd, &dir, sizeof(struct dirent)) == sizeof(struct dirent)) {
      if (dir.d_ino != 0 && dir.d_name[0] != '.') {
        if (needle_len == 0 || strncmp(dir.d_name, needle, needle_len) == 0) {

          struct stat st;
          char filename[PATH_MAX] = {0};
          snprintf(filename, PATH_MAX, "%s/%s", dirname, dir.d_name);
          if (stat(filename, &st) == 0 && S_ISDIR(st.st_mode)) {
            snprintf(filename, PATH_MAX, "%s/", dir.d_name);
            matches[(*matches_size)++] = strdup(filename);
          } else {
            matches[(*matches_size)++] = strdup(dir.d_name);
          }
        }
      }
    }
    close(fd);
  }
}

unsigned int find_forward_match_end(unsigned int forward_match_start, char **matches, int matches_size) {
  if (matches_size == 0)
    return forward_match_start;

  unsigned int end = strlen(matches[0]);
  for (int m = 1; m < matches_size; m++) {
    unsigned int f = forward_match_start;
    while (matches[0][f] && matches[m][f] && matches[0][f] == matches[m][f] && matches[m][f] != '/')
      f++;

    if (f < end) {
      end = f;
    }
  }
  return end;
}

void complete_file(int path_start, int word_start, unsigned int forward_match_start, char *cmdline, int *cursor_pos,
                   int *cmdline_size) {
  char needle[PATH_MAX];
  char path[PATH_MAX];
  strncpy(path, cmdline + path_start, word_start - path_start);
  path[word_start - path_start] = 0;
  if (*path == 0)
    strncpy(path, "./", PATH_MAX);
  strncpy(needle, cmdline + word_start, *cursor_pos - word_start);
  needle[*cursor_pos - word_start] = '\0';

  char *matches[64];
  int matches_size = 0;
  find_matches(path, basename(needle), matches, &matches_size);

  if (matches_size > 0) {
    qsort(matches, matches_size, sizeof(char *), compare);

    unsigned int forward_match_end = find_forward_match_end(forward_match_start, matches, matches_size);
    if (forward_match_end > forward_match_start) {
      // Add forward matching characters
      for (unsigned int f = forward_match_start; f < forward_match_end; f++) {
        insert(cmdline, matches[0][f], cursor_pos, cmdline_size);
      }

      if (forward_match_end == strlen(matches[0]) && matches[0][forward_match_end - 1] != '/') {
        insert(cmdline, ' ', cursor_pos, cmdline_size);
      }
    } else {
      // Print matches
      printf("\n\033[2K");
      for (int i = 0; i < matches_size; i++) {
        printf("%s ", matches[i]);
      }
      printf("\n");
    }
  }

  for (int i = 0; i < matches_size; i++) {
    free(matches[i]);
  }
}

void complete_command(int start_pos, unsigned int forward_match_start, char *cmdline, int *cursor_pos,
                      int *cmdline_size) {
  int matches_size = 0;
  char *matches[64];
  char *path = strdup(getenv("PATH"));
  char needle[PATH_MAX];
  strncpy(needle, cmdline + start_pos, *cursor_pos - start_pos);
  needle[*cursor_pos - start_pos] = '\0';

  char *token = strtok(path, ":");
  while (token) {
    find_matches(token, needle, matches, &matches_size);
    token = strtok(NULL, ":");
  }

  if (matches_size > 0) {
    qsort(matches, matches_size, sizeof(char *), compare);

    unsigned int forward_match_end = find_forward_match_end(forward_match_start, matches, matches_size);
    if (forward_match_end > forward_match_start) {
      // Add forward matching characters
      for (unsigned int f = forward_match_start; f < forward_match_end; f++) {
        insert(cmdline, matches[0][f], cursor_pos, cmdline_size);
      }
      if (forward_match_end == strlen(matches[0])) {
        insert(cmdline, ' ', cursor_pos, cmdline_size);
      }
    } else {
      // Print matches
      printf("\n\033[2K");
      for (int i = 0; i < matches_size; i++) {
        printf("%s ", matches[i]);
      }
      printf("\n");
    }
  }
  free(path);
}

int main(int argc, char *argv[]) {
  signal(SIGINT, sighandler);

  FILE *input = stdin;

  if (argc >= 2) {
    if (strcmp(argv[1], "-") != 0) {
      input = fopen(argv[1], "r");
      if (input == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
      }
      interactive = 0;
    }
  }

  if (interactive && getuid() == 0) {
    strncpy(prompt, "# ", PROMPT_MAX_SIZE);
  }

  atexit(canonical_mode);

  // TODO: IFS handling: Support for setting and using IFS to control word splitting behavior
  // TODO: Sub shell $(cmd): Execute cmd in a subshell and replace with its output
  char cmdline[MAX_LINE];
  const char *history[HISTORY_MAX_SIZE] = {};
  int history_index = 0;
  int history_size = 0;

  int cursor_pos = 0;
  int cmdline_size = 0;
  cmdline[0] = 0;
  redraw(prompt, cmdline, cursor_pos, cmdline_size);

  cbreak_mode();

  while (1) {
    int c = fgetc(input);
    if (c == EOF)
      break;

    if (c == 27) {
      c = fgetc(input);
      if (c == EOF)
        break;

      if (c == 91) {
        c = fgetc(input);
        if (c == EOF)
          break;

        if (c == 51) {
          c = fgetc(input);
          if (c == EOF)
            break;

          if (c == 126) { // Delete
            if (cursor_pos < cmdline_size) {
              delete(cmdline, cursor_pos, &cmdline_size);
              redraw(prompt, cmdline, cursor_pos, cmdline_size);
            }
          }
        } else if (c == 65) { // Up arrow
          if (history_index > 0) {
            history_index--;
            history_copy(cmdline, history, history_index, history_size, &cursor_pos, &cmdline_size);
            redraw(prompt, cmdline, cursor_pos, cmdline_size);
          }
        } else if (c == 66) { // Down arrow
          if (history_index < history_size) {
            history_index++;
            history_copy(cmdline, history, history_index, history_size, &cursor_pos, &cmdline_size);
            redraw(prompt, cmdline, cursor_pos, cmdline_size);
          }
        } else if (c == 67) { // Right arrow
          if (cursor_pos < cmdline_size) {
            cursor_pos++;
            if (interactive) {
              printf("\033[C");
              fflush(stdout);
            }
          }
        } else if (c == 68) { // Left arrow
          if (cursor_pos > 0) {
            cursor_pos--;
            if (interactive) {
              printf("\b");
              fflush(stdout);
            }
          }
        } else if (c == 72) { // Home key
          cursor_pos = 0;
          redraw(prompt, cmdline, cursor_pos, cmdline_size);
        } else if (c == 70) { // End key
          cursor_pos = cmdline_size;
          redraw(prompt, cmdline, cursor_pos, cmdline_size);
        }
      }
    } else if (c == 12) { // Ctrl+L, clear screen
      if (interactive) {
        printf("\033[H\033[J");
        redraw(prompt, cmdline, cursor_pos, cmdline_size);
      }
    } else if (c == 4) { // Ctrl+D, end of input
      break;
    } else if (c == 9) { // Tab key
      int i = 0;
      // skip leading spaces before the cursor
      while (cmdline[i] == ' ' && i < cursor_pos)
        i++;

      // count words before the cursor
      int word_count = 0;
      int word_start = 0;
      int word_char = 0;
      int path_start = 0;
      while (i < cursor_pos) {
        word_start = i;
        word_char = 0;
        while (i < cursor_pos && cmdline[i] != ' ' && cmdline[i] != '/') {
          i++;
          word_char++;
        }

        if (cmdline[i] == '/') {
          i++;
          word_start = i;
        }
        word_count++;

        while (i < cursor_pos && cmdline[i] == ' ') {
          i++;
          path_start = i;
        }
      }
      if (word_start < path_start) {
        word_start = path_start;
        word_count++;
      }

      unsigned int forward_match_start = word_char;
      if (word_count < 2) {
        // Complete command
        complete_command(word_start, forward_match_start, cmdline, &cursor_pos, &cmdline_size);
      } else {
        // Complete file
        complete_file(path_start, word_start, forward_match_start, cmdline, &cursor_pos, &cmdline_size);
      }
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == 0x17) { // Ctrl+W, delete word before cursor
      delete_word(cmdline, &cursor_pos, &cmdline_size);
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == 127) { // Backspace
      delete_char(cmdline, &cursor_pos, &cmdline_size);
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == 21) { // Ctrl+U, clear line
      clear_line(cmdline, &cursor_pos, &cmdline_size);
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == 1) { // Ctrl+A, move cursor to beginning of line
      cursor_pos = 0;
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == 5) { // Ctrl+E, move cursor to end of line
      cursor_pos = cmdline_size;
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (c == '\n' || c == '\r') {
      // Not an escape sequence, handle normally
      if (interactive) {
        printf("\n");
        fflush(stdout);
      }

      history_save(cmdline, history, &history_index, &history_size);
      canonical_mode();
      execute_command(cmdline);
      cbreak_mode();

      cmdline[0] = 0;
      cmdline_size = cursor_pos = 0;
      redraw(prompt, cmdline, cursor_pos, cmdline_size);
    } else if (isprint(c)) {
      if (cursor_pos == cmdline_size) {
        insert(cmdline, c, &cursor_pos, &cmdline_size);
        if (interactive) {
          printf("%c", c);
          fflush(stdout);
        }
      } else {
        insert(cmdline, c, &cursor_pos, &cmdline_size);
        redraw(prompt, cmdline, cursor_pos, cmdline_size);
      }
    } else {
      // Unhandled character, ignore it
    }
  }

  if (!interactive) {
    fclose(input);
  } else {
    printf("exit\n");
  }
  return EXIT_SUCCESS;
}
