// *** flash-vars.c ***************************************************
// SPDX-License-Identifier: Parity-7.0.0
// BASIC-compatible flash variables; see flash-vars.h. Messages match the
// StickOS interpreter's.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "params.h"
#include "flash-vars.h"

#define MAX_VARS  32
#define MAX_NAME  31

static struct var {
    char name[MAX_NAME+1];
    uint32_t offset;     // in the parameter page's user words
    int32_t max_index;   // number of elements
} vars[MAX_VARS];

static int nvars;
static uint32_t next_offset;

// output buffer for one command line
static char *out;
static int out_left;
static char *out_start;
static int out_size;

static void
put(const char *s)
{
    while (*s && out_left > 1) {
        *out++ = *s++;
        out_left--;
    }
    *out = '\0';
}

static void
put_int(int32_t value)
{
    char digits[12];
    char *p;
    uint32_t u;

    p = digits+sizeof(digits)-1;
    *p = '\0';
    u = value < 0 ? -(uint32_t)value : (uint32_t)value;
    do {
        *--p = (char)('0'+u%10);
        u /= 10;
    } while (u);
    if (value < 0) {
        *--p = '-';
    }
    put(p);
}

static void
skip_space(const char **text)
{
    while (**text == ' ' || **text == '\t') {
        (*text)++;
    }
}

// matches word followed by a non-identifier character
static bool
parse_word(const char **text, const char *word)
{
    size_t n;
    char c;

    n = strlen(word);
    if (strncmp(*text, word, n)) {
        return false;
    }
    c = (*text)[n];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
        return false;
    }
    *text += n;
    skip_space(text);
    return true;
}

static bool
parse_char(const char **text, char c)
{
    if (**text != c) {
        return false;
    }
    (*text)++;
    skip_space(text);
    return true;
}

static bool
parse_name(const char **text, char *name)
{
    int n;
    char c;

    n = 0;
    for (;;) {
        c = (*text)[n];
        if (! ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (n && c >= '0' && c <= '9'))) {
            break;
        }
        if (n == MAX_NAME) {
            return false;
        }
        name[n++] = c;
    }
    if (! n) {
        return false;
    }
    name[n] = '\0';
    *text += n;
    skip_space(text);
    return true;
}

// decimal or 0x hex, optionally negative
static bool
parse_int(const char **text, int32_t *value)
{
    const char *p;
    bool negative;
    uint32_t v;
    int digits;
    char c;

    p = *text;
    negative = *p == '-';
    if (negative) {
        p++;
    }
    v = 0;
    digits = 0;
    if (p[0] == '0' && p[1] == 'x') {
        p += 2;
        for (;; p++, digits++) {
            c = *p;
            if (c >= '0' && c <= '9') {
                v = v*16+(uint32_t)(c-'0');
            } else if (c >= 'a' && c <= 'f') {
                v = v*16+(uint32_t)(c-'a'+10);
            } else {
                break;
            }
        }
    } else {
        for (; *p >= '0' && *p <= '9'; p++, digits++) {
            v = v*10+(uint32_t)(*p-'0');
        }
    }
    if (! digits) {
        return false;
    }
    *value = (int32_t)(negative ? -v : v);
    *text = p;
    skip_space(text);
    return true;
}

#define NO_INDEX  INT32_MIN  // <name> without [<int>]

// <name>[[<int>]]; index is NO_INDEX if absent
static bool
parse_ref(const char **text, char *name, int32_t *index)
{
    if (! parse_name(text, name)) {
        return false;
    }
    *index = NO_INDEX;
    if (parse_char(text, '[')) {
        if (! parse_int(text, index) || ! parse_char(text, ']')) {
            return false;
        }
    }
    return true;
}

static struct var *
find(const char *name)
{
    int i;

    for (i = 0; i < nvars; i++) {
        if (! strcmp(vars[i].name, name)) {
            return vars+i;
        }
    }
    return NULL;
}

// finds name and checks index; reports errors to out
static struct var *
lookup(const char *name, int32_t index)
{
    struct var *var;

    var = find(name);
    if (! var) {
        put("var '"); put(name); put("' undefined\n");
        return NULL;
    }
    if (index != NO_INDEX && (index < -1 || index >= var->max_index)) {
        put("var '"); put(name); put("' index "); put_int(index); put(" out of range\n");
        return NULL;
    }
    return var;
}

static void
declare(const char *name, int32_t max_index)
{
    struct var *var;

    if (! max_index) {
        put("declared 0 length array\n");
        return;
    }
    if (find(name)) {
        put("var '"); put(name); put("' already declared at this scope\n");
        return;
    }
    if (nvars == MAX_VARS) {
        put("out of variables\n");
        return;
    }
    if (max_index < 0 || (uint32_t)max_index > (PARAMS_USER_BYTES-next_offset)/sizeof(uint32_t)) {
        put("out of parameter flash\n");
        return;
    }

    var = vars+nvars++;
    strcpy(var->name, name);
    var->offset = next_offset;
    var->max_index = max_index;
    next_offset += (uint32_t)max_index*sizeof(uint32_t);
}

static uint32_t
element(const struct var *var, int32_t index)
{
    return var->offset+(uint32_t)(index < 0 ? 0 : index)*sizeof(uint32_t);
}

void
flashvars_reset(void)
{
    nvars = 0;
    next_offset = 0;
}

enum statement { statement_dim, statement_print, statement_let };

// the line a print statement builds; StickOS prints error messages as they
// occur and the line after them
static char print_line[160];

// directs put() to the end of print_line (true) or back to the reply (false)
static void
to_print_line(bool on)
{
    static char *reply_out;
    static int reply_left;
    int n;

    if (on) {
        reply_out = out;
        reply_left = out_left;
        n = (int)strlen(print_line);
        out = print_line+n;
        out_left = (int)sizeof(print_line)-n;
    } else {
        out = reply_out;
        out_left = reply_left;
    }
}

// parses (and, if execute, runs) the statement after its keyword; returns
// FLASHVARS_OK or the offset of a syntax error. As in StickOS, the whole
// line is parsed before anything runs, and a runtime error in one item
// prints a message without stopping the others: print shows nothing for an
// undefined variable without index and 0 for any other failed reference, and an assignment to
// either is skipped.
static int
statement(enum statement kind, const char *line, const char *text, bool execute)
{
    char name[MAX_NAME+1];
    int32_t index;
    int32_t value;
    int32_t i;
    struct var *var;
    bool first;

    print_line[0] = '\0';
    first = true;
    do {
        if (! parse_ref(&text, name, &index)) {
            return (int)(text-line);
        }
        switch (kind) {
            case statement_dim:
                if (! parse_word(&text, "as") || ! parse_word(&text, "flash")) {
                    return (int)(text-line);
                }
                // every declaration is attempted; failures only print a message
                if (execute) {
                    declare(name, index == NO_INDEX ? 1 : index);
                }
                break;

            case statement_print:
                if (execute) {
                    var = lookup(name, index);
                    to_print_line(true);
                    if (! first) {
                        put(" ");
                    }
                    if (! var) {
                        // as StickOS: an indexed reference that fails reads
                        // 0, an undefined name without index nothing
                        if (index != NO_INDEX) {
                            put("0");
                        }
                    } else if (index == NO_INDEX) {
                        // a whole array: every element
                        for (i = 0; i < var->max_index; i++) {
                            if (i) {
                                put(" ");
                            }
                            put_int(params_get_user(element(var, i)));
                        }
                    } else {
                        put_int(params_get_user(element(var, index)));
                    }
                    to_print_line(false);
                }
                break;

            case statement_let:
                if (! parse_char(&text, '=') || ! parse_int(&text, &value)) {
                    return (int)(text-line);
                }
                if (execute) {
                    var = lookup(name, index);
                    if (var) {
                        params_set_user(element(var, index), value);
                    }
                }
                break;
        }
        first = false;
    } while (parse_char(&text, ','));

    if (*text) {
        return (int)(text-line);
    }
    if (execute && kind == statement_print) {
        put(print_line);
        put("\n");
    }
    return FLASHVARS_OK;
}

int
flashvars_run(const char *line, char *reply, int reply_size)
{
    const char *text;
    const char *probe;
    char name[MAX_NAME+1];
    int32_t index;
    enum statement kind;
    int rv;

    out = out_start = reply;
    out_left = out_size = reply_size;
    put("");

    text = line;
    skip_space(&text);

    if (parse_word(&text, "dim")) {
        kind = statement_dim;
    } else if (parse_word(&text, "print")) {
        kind = statement_print;
    } else if (parse_word(&text, "let")) {
        kind = statement_let;
    } else {
        // without let, only <ref> = is an assignment
        probe = text;
        if (! parse_ref(&probe, name, &index) || *probe != '=') {
            return FLASHVARS_NOT_MINE;
        }
        kind = statement_let;
    }

    rv = statement(kind, line, text, false);
    if (rv != FLASHVARS_OK) {
        return rv;
    }
    return statement(kind, line, text, true);
}
