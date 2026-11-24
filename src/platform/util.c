// *** util.c *********************************************************
// this file implements generic utility functions.

#include "main.h"

// return the current interrupt mask level
int
gpl(void)
{
    int oldlevel;

    oldlevel = (int)((_CP0_GET_STATUS() >> 10) & 7);
    return oldlevel;
}

// set the current interrupt mask level and return the old one
int
splx(int level)
{
    int csr;
    int oldlevel;

    // get the sr
    csr = (int)_CP0_GET_STATUS();


    oldlevel = (csr >> 10) & 7;
    if (level <= 0) {
        // we're going down
        level = -level;
    } else {
        // we're going up
        level = MAX(level, oldlevel);
    }
    assert(level >= 0 && level <= 7);
    // cppcheck-suppress shiftNegativeLHS ; level is 0..7 (asserted above): -level of a level <= 0, or MAX() of a level > 0
    csr = (int)(((uint32)csr & 0xffffe3ff) | (uint32)(level << 10));

    // update the sr
    _CP0_SET_STATUS((uint32)csr);

    assert(oldlevel >= 0 && oldlevel <= 7);
    return -oldlevel;
}

static volatile int g;

int blips_per_ms;

void
blip(void)
{
    int x;
    for (x = 0; x < 500; x++) {
        g++;
    }
}

// delay for the specified number of milliseconds
void
delay(int32 ms)
{
    int x;
    int32 m;
    int blips;

    // if interrupts are initialized...
    if (gpl() < SPL_PIT0) {
        // wait for the pit0 to count off the ticks
        m = msecs;
        blips = 0;
        while (msecs-m < ms+1) {
            blip();
            blips++;
        }
        if (! blips_per_ms) {
            blips_per_ms = blips/ms+1;
        }
    // otherwise; make a good guess with a busywait
    } else {
        assert(blips_per_ms);
        while (ms--) {
            for (x = 0; x < blips_per_ms; x++) {
                blip();
            }
        }
    }
}

int
gethex(const char *p)
{
    char c;

    c = *p;
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else {
        return -1;
    }
}

int
get2hex(char **p)
{
    int v1, v0;

    v1 = gethex(*p);
    if (v1 == -1) {
        return -1;
    }
    v0 = gethex(*p+1);
    if (v0 == -1) {
        return -1;
    }

    (*p) += 2;
    return v1*16+v0;
}

void *
memcpy(void *d,  const void *s, size_t n)
{
    if (((uintptr)d&3)+((uintptr)s&3)+(n&3) == 0) {
        uint32 *dtemp = d;
        const uint32 *stemp = s;

        n >>= 2;
        while (n--) {
            *(dtemp++) = *(stemp++);
        }
    } else {
        uint8 *dtemp = d;
        const uint8 *stemp = s;

        while (n--) {
            *(dtemp++) = *(stemp++);
        }
    }
    return d;
}

void *
memmove(void *d,  const void *s, size_t n)
{
    void *dd;

    if ((char *)d > (const char *)s && (char *)d < (const char *)s+n) {
        dd = d;
        while (n--) {
            *((char *)d+n) = *((const char *)s+n);
        }
        return dd;
    } else {
        return memcpy(d, s, n);
    }
}

void *
memset(void *p,  int d, size_t n)
{
    int dd;

    if (((uintptr)p&3)+(n&3) == 0) {
        uint32 *ptemp = p;

        n >>= 2;
        d = d & 0xff;
        dd = d<<24|d<<16|d<<8|d;
        while (n--) {
            *(ptemp++) = (uint32)dd;
        }
    } else {
        uint8 *ptemp = p;

        while (n--) {
            *(ptemp++) = (uint8)d;
        }
    }
    return p;
}

int
memcmp(const void *d,  const void *s, size_t n)
{
    int c;
    const uint8 *dtemp = d;
    const uint8 *stemp = s;

    while (n--) {
        c = *(dtemp++) - *(stemp++);
        if (c) {
            return c;
        }
    }
    return 0;
}

size_t
strlen(const char *s)
{
    const char *stemp = s;

    while (*stemp) {
        stemp++;
    }
    return (size_t)(stemp - s);
}

char *
strcat(char *dest, const char *src)
{
    char *orig_dest = dest;

    while (*dest) {
        dest++;
    }
    do {
        *(dest++) = *src;
    } while (*(src++));

    return orig_dest;
}

char *
strcpy(char *dest, const char *src)
{
    char *orig_dest = dest;

    do {
        *(dest++) = *src;
    } while (*(src++));

    return orig_dest;
}

char *
strncpy(char *dest, const char *src, size_t n)
{
    char *orig_dest = dest;

    while (n--) {
        *(dest++) = *src;
        if (! *(src++)) {
            break;
        }
    }

    return orig_dest;
}

int
strcmp(const char *s1, const char *s2)
{
    for (; *s1 == *s2; s1++, s2++) {
        if (! *s1) {
            return 0;
        }
    }

    if (*s1 < *s2) {
        return -1;
    } else {
        return 1;
    }
}

int
strncmp(const char *s1, const char *s2, size_t n)
{
    for (; n && (*s1 == *s2); n--, s1++, s2++) {
        if (! *s1) {
            return 0;
        }
    }

    if (n == 0) {
        return 0;
    }

    if (*s1 < *s2) {
        return -1;
    } else {
        return 1;
    }
}

char *
strchr(const char *s, int c)
{
// the C library signature returns a non-const pointer into a const string
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    for (; *s; s++) {
        if (*s == c) {
            return (char *)s;
        }
    }
    if (*s == c) {
        return (char *)s;
    }
#pragma GCC diagnostic pop
    return NULL;
}

int
isdigit(int c)
{
    return (c >= '0') && (c <= '9');
}

int
isspace(int c)
{
    return (c == ' ') || (c == '\f') || (c == '\n') || (c == '\r') || (c == '\t') || (c == '\v');
}

int
isprint(int c)
{
    return (c >= ' ') && (c <= '~');
}



static uint32 m_w = 1;    /* must not be zero */
static uint32 m_z = 2;    /* must not be zero */

uint32
random_32(
    void
    )
{
    uint32 z;
    uint32 w;

    z = m_z;
    w = m_w;
    assert(z);
    assert(w);
    m_z = z = 36969 * (z & 65535) + (z >> 16);
    m_w = w = 18000 * (w & 65535) + (w >> 16);
    return (z << 16) + w;  /* 32-bit result */
}
