#define ROWS 25
#define MAX_ROWS 1024
#define MAX_COLS 256

extern char buffer[MAX_ROWS][MAX_COLS];

enum Mode { ModeEx = 0, ModeCommand, ModeCommandLine, ModeInsert, ModeReplaceChar };

extern enum Mode mode;
extern int row;
extern int col;
extern int row_offset;
extern int lines;

extern char message[32];

void clear_screen(void);
void draw_screen();
void disable_raw_mode();
void enable_raw_mode();

void load_file(const char *pathname);
void move_cursor(int r, int c);
void draw_line(int row);
void draw_status(void);
void delete_lines(int row, int count);
void delete_word(int row, int col, int count, char remove_whitespace_after);
void quit();
void save();
void save_as(const char *pathname);

void insert_mode_input(char c);
void command_mode_input(char c);
void commandline_mode_input(char c);
