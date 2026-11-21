// *** guestio.h ******************************************************
// console I/O for STICK_GUEST builds, implemented in stickos/guest-main.c, so
// that files which use names such as open() as identifiers need not include
// <unistd.h>.

// this function returns true if stdin is a terminal rather than a file or pipe.
bool
stdin_is_tty(void);

// this function writes length bytes of buffer to stdout, best effort.
void
console_write(const char *buffer, int length);

// this function reads one line from stdin into buffer without the trailing newline,
// discarding any part of the line that does not fit; it returns NULL at end of file.
char *
read_line(char *buffer, int size);
