typedef void (*terminal_command_cbfn)(char *command);
typedef void (*terminal_ctrlc_cbfn)(void);

extern bool terminal_echo;

void
terminal_print(const byte *buffer, int length);

bool
terminal_receive(byte *buffer, int length);

void
terminal_command_discard(bool discard);

void
terminal_command_ack(bool edit);

void
terminal_command_ack_discard(void);

void
terminal_command_error(int offset);

void
terminal_poll(void);

void
terminal_register(terminal_command_cbfn command_in, terminal_ctrlc_cbfn ctrlc);
