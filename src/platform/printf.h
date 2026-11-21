// *** printf.h *******************************************************

int
printf_write(char *buffer, int n);

bool
open_log_file(void);

bool
append_log_file(char *buffer);

void
flush_log_file(void);

// main.h renames these to my*, so the compiler doesn't know them as the
// printf family: declare their format strings for -Wformat. __printf__,
// since main.h #defines printf.
int
vsnprintf(char *buffer, size_t length, const char *format, va_list ap)
    __attribute__((format(__printf__, 3, 0)));

int
snprintf(char *buffer, size_t length, const char *format, ...)
    __attribute__((format(__printf__, 3, 4)));

int
sprintf(char *buffer, const char *format, ...)
    __attribute__((format(__printf__, 2, 3)));

int
vsprintf(char *buffer, const char *format, va_list ap)
    __attribute__((format(__printf__, 2, 0)));

int
printf(const char *format, ...)
    __attribute__((format(__printf__, 1, 2)));

int
vprintf(const char *format, va_list ap)
    __attribute__((format(__printf__, 1, 0)));

#if IN_MEMORY_TRACE

void
trace(const char *fmt, ...)
    __attribute__((format(__printf__, 1, 2)));

void
trace_reset(void);

#endif // IN_MEMORY_TRACE
