// *** util.h *********************************************************

// XXX -- fix all this!!!
#define SPL_PIT0  4  // pit0 isr runs at interrupt level 4
#define SPL_USB  4  // usb isr runs at interrupt level 4
#define SPL_IRQ4  4  // irq isr runs at interrupt level 4 (zigflea)
#define SPL_SERIAL  4  // serial console

// N.B. the usb controller bdt data structures and the usb protocol
// layers are defined to be little endian, as is the pic32 core.
#define TF_LITTLE(x)  (x)

// return the current interrupt mask level
int
gpl(void);

// set the current interrupt mask level and return the old one
int
splx(int level);

extern int blips_per_ms;

void
blip(void);

// delay for the specified number of milliseconds
void
delay(int32 ms);

int
gethex(const char *p);

int
get2hex(char **p);

void *
memcpy(void *d,  const void *s, size_t n);

void *
memmove(void *d,  const void *s, size_t n);

void *
memset(void *p,  int d, size_t n);

int
memcmp(const void *d,  const void *s, size_t n);

size_t
strlen(const char *s);

char *
strcat(char *dest, const char *src);

char *
strcpy(char *dest, const char *src);

char *
strncpy(char *dest, const char *src, size_t n);

int
strcmp(const char *s1, const char *s2);

int
strncmp(const char *s1, const char *s2, size_t n);

char *
strchr(const char *s, int c);

int
isdigit(int c);

int
isspace(int c);

int
isprint(int c);

uint32
random_32(
    void
    );
