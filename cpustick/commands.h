// *** commands.h *****************************************************

extern char *volatile main_command;
extern bool main_prompt;

// cleared by Ctrl-C (stop()) to end a running capture or stream
extern volatile bool run_stop;

extern const char * const help_about;

void
stop(void);

void
parse_trim(char **text);

// matches word as a prefix of *text and skips it and following spaces
bool
parse_word(char **text, const char *word);

void
main_run(void);

void
main_initialize(void);
