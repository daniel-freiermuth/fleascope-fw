// *** guestio.h ******************************************************
// console I/O for STICK_GUEST builds, implemented in stickos/w32main.c.
// N.B. only w32main.c includes the POSIX headers: <io.h> and <unistd.h> declare
// names such as open() and umask() that other files use as identifiers, and on
// Windows _CRT_NONSTDC_NO_WARNINGS must precede every CRT header to silence the
// POSIX-name deprecation of isatty() and write().

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
