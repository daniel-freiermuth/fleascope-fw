// *** commands.h *****************************************************
// SPDX-License-Identifier: Parity-7.0.0

extern char *volatile main_command;
extern bool main_prompt;

// cleared by Ctrl-C (stop()) to end a running capture or stream
extern volatile bool run_stop;

extern const char * const help_about;

void
stop(void);

void
parse_trim(const char **text);

// matches word as a prefix of *text and skips it and following spaces
bool
parse_word(const char **text, const char *word);

void
__attribute__((noreturn))
main_run(void);

void
main_initialize(void);
