// *** commands.c *****************************************************
// SPDX-License-Identifier: Parity-7.0.0
// the Flea-Scope command line: the main loop that waits for command lines
// from the terminal and runs them. It answers in the format the StickOS
// interpreter used, so existing hosts keep working:
//
//   scope, stream, digital_stream, wave    capture and signal generation
//   ver, hostname, reset, upgrade, help
//   echo [on|off], prompt [on|off]
//
// after each command line comes "> " (prompt on) or "done\n" (prompt off).

#include "main.h"

char *volatile main_command;
bool main_prompt = true;
volatile bool run_stop;

const char * const help_about =
"Welcome to the Flea-Scope firmware v" VERSION "!\n"
"https://github.com/daniel-freiermuth/fleascope-fw\n"
;

static const char * const help_commands =
"scope <qus> [+-~](<level>|0x<bits>) [<slew>|0x<mask>] [<delay>]\n"
"stream [<timer ticks> [<averaging> [<bitwidth>]]]\n"
"digital_stream 0x<mask> [<us_per_sample> [<n_samples_per_packet>]]\n"
"wave (square|ekg|sine|triangle) <hz>\n"
"ver, hostname [<name>], reset, upgrade\n"
"echo [on|off], prompt [on|off]\n"
;

void
stop(void)
{
    run_stop = false;
}

void
parse_trim(const char **text)
{
    while (**text == ' ' || **text == '\t') {
        (*text)++;
    }
}

bool
parse_word(const char **text, const char *word)
{
    size_t len;

    len = strlen(word);
    if (strncmp(*text, word, len) != 0 || ((*text)[len] && (*text)[len] != ' ' && (*text)[len] != '\t')) {
        return false;
    }
    *text += len;
    parse_trim(text);
    return true;
}

// a decimal or 0x hex constant, as StickOS's run_input_const()
static bool
parse_const(const char **text, int *value_out)
{
    int value;
    char c;

    if (! isdigit((*text)[0])) {
        return false;
    }
    value = 0;
    if ((*text)[0] == '0' && (*text)[1] == 'x') {
        *text += 2;
        for (;;) {
            c = (*text)[0];
            if (c >= 'a' && c <= 'f') {
                value = value*16 + 10 + c - 'a';
            } else if (isdigit(c)) {
                value = value*16 + c - '0';
            } else {
                break;
            }
            (*text)++;
        }
    } else {
        while (isdigit((*text)[0])) {
            value = value*10 + (*text)[0] - '0';
            (*text)++;
        }
    }
    *value_out = value;
    parse_trim(text);
    return true;
}

// a command name, ending at a space or the end of the line
static bool
parse_command(const char **text, const char *name)
{
    size_t len;

    len = strlen(name);
    if (strncmp(*text, name, len) != 0 || ((*text)[len] && (*text)[len] != ' ' && (*text)[len] != '\t')) {
        return false;
    }
    *text += len;
    parse_trim(text);
    return true;
}

// echo/prompt [on|off]; returns false on a syntax error
static bool
mode(const char **text, bool *var)
{
    if (! **text) {
        printf("%s\n", *var ? "on" : "off");
        return true;
    }
    if (parse_word(text, "on")) {
        *var = true;
    } else if (parse_word(text, "off")) {
        *var = false;
    } else {
        return false;
    }
    return ! **text;
}

static bool
command_scope(const char **text)
{
    int number1;
    int number2;
    int number3;
    int number4;
    TriggerEdge edge;
    TriggerChannel channel;

    number1 = 10;
    number2 = -1;
    number3 = -1;
    number4 = 0;
    if (**text && (! parse_const(text, &number1) || number1 <= 0)) {
        return false;
    }
    if (**text == '+') {
        edge = RISE_AKA_START;
        (*text)++;
    } else if (**text == '-') {
        edge = FALL_AKA_STOP;
        (*text)++;
    } else if (**text == '~') {
        edge = AUTO;
        (*text)++;
    } else {
        edge = LEVEL_AKA_MATCHING;
    }
    channel = (**text == '0' && (*text)[1] == 'x') ? DIGITAL : ANALOG;
    if (**text && (! parse_const(text, &number2) || number2 &~ 1023)) {
        return false;
    }
    if (**text && (! parse_const(text, &number3) || number3 &~ 1023)) {
        return false;
    }
    if (**text && (! parse_const(text, &number4) || number4 < 0 || number4 > 1000000)) {
        return false;
    }
    if (**text) {
        return false;
    }
    scope(number1, edge, channel, number2, number3, number4);
    return true;
}

static bool
command_stream(const char **text)
{
    int timer_period_ticks = 2333;  // ~51 kHz (bus_frequency/1000000 * 70 * INTERLEAVE / MSPS)
    int averaging = 1;              // no averaging
    int bitwidth = 16;              // two-byte little-endian samples

    if (**text && (! parse_const(text, &timer_period_ticks) ||
                   timer_period_ticks < 2 || timer_period_ticks > 65535)) {
        return false;
    }
    if (**text && (! parse_const(text, &averaging) || averaging < 1 || averaging > 5)) {
        return false;
    }
    if (**text && (! parse_const(text, &bitwidth) || bitwidth < 1 || bitwidth > 16)) {
        return false;
    }
    if (**text) {
        return false;
    }
    stream((uint)timer_period_ticks, (uint)averaging, (uint)bitwidth);
    return true;
}

static bool
command_digital_stream(const char **text)
{
    int mask;
    int us_per_sample;
    int n_samples_per_packet;

    if (! (**text == '0' && (*text)[1] == 'x')) {
        return false;
    }
    if (! parse_const(text, &mask) || mask == 0 || (mask & ~0x1ff)) {
        return false;
    }
    us_per_sample = 0;         // free-run
    n_samples_per_packet = 0;  // the most that fits one packet
    if (**text && (! parse_const(text, &us_per_sample) || us_per_sample < 0)) {
        return false;
    }
    if (**text && (! parse_const(text, &n_samples_per_packet) || n_samples_per_packet <= 0)) {
        return false;
    }
    if (**text) {
        return false;
    }
    digital_stream(mask, us_per_sample, n_samples_per_packet);
    return true;
}

// sized by its initialisers so the CASSERT below fails to compile when a
// WaveShape is appended without a name
static const char * const wave_names[] = {
    [WAVE_SQUARE] = "square",
    [WAVE_EKG] = "ekg",
    [WAVE_SINE] = "sine",
    [WAVE_TRIANGLE] = "triangle",
};
CASSERT(LENGTHOF(wave_names) == WAVE_COUNT);

static bool
command_wave(const char **text)
{
    WaveShape shape;
    int hz;

    for (shape = 0; shape < WAVE_COUNT; shape++) {
        // a name missing from the middle of the table is NULL; skip it
        // rather than hand it to parse_word()
        if (wave_names[shape] && parse_word(text, wave_names[shape])) {
            break;
        }
    }
    if (shape == WAVE_COUNT || ! parse_const(text, &hz) || **text) {
        return false;
    }
    if (hz == 0) {
        wave_off();
    } else if (! wave(shape, hz)) {
        printf("bad frequency\n");
    }
    return true;
}

static void
reset(void)
{
    (void)splx(7);
    SYSKEY = 0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    RSWRSTSET = _RSWRST_SWRST_MASK;
    while (true) {
        (void)RSWRST;  // reading RSWRST triggers the reset
    }
}

// prints text line by line. printf() turns '\n' into "\r\n" only in the
// format string and cuts each %s at BASIC_OUTPUT_LINE_SIZE characters, so
// multi-line text must not go through a single "%s".
static void
print_lines(const char *text)
{
    char line[BASIC_OUTPUT_LINE_SIZE+1];
    const char *end;
    int n;

    while (*text) {
        end = strchr(text, '\n');
        n = end ? end-text : (int)strlen(text);
        assert(n <= BASIC_OUTPUT_LINE_SIZE);
        n = MIN(n, BASIC_OUTPUT_LINE_SIZE);
        memcpy(line, text, (size_t)n);
        line[n] = '\0';
        printf("%s\n", line);
        text = end ? end+1 : text+strlen(text);
    }
}

// runs one command line
static void
run(const char *text_in)
{
    const char *text;
    bool ok;

    text = text_in;
    parse_trim(&text);

    ok = true;
    if (! *text) {
        // NULL
    } else if (parse_command(&text, "scope")) {
        ok = command_scope(&text);
    } else if (parse_command(&text, "stream")) {
        ok = command_stream(&text);
    } else if (parse_command(&text, "digital_stream")) {
        ok = command_digital_stream(&text);
    } else if (parse_command(&text, "wave")) {
        ok = command_wave(&text);
    } else if (parse_command(&text, "ver")) {
        ok = ! *text;
        if (ok) {
            printf(VERSION " (checksum 0x%x)\n", flash_checksum);
        }
    } else if (parse_command(&text, "hostname")) {
        if (*text) {
            params_set_name(text);
        } else {
            printf("%s\n", params_get_name());
        }
    } else if (parse_command(&text, "echo")) {
        ok = mode(&text, &terminal_echo);
    } else if (parse_command(&text, "prompt")) {
        ok = mode(&text, &main_prompt);
    } else if (parse_command(&text, "reset")) {
        ok = ! *text;
        if (ok) {
            reset();
        }
    } else if (parse_command(&text, "upgrade")) {
        ok = ! *text;
        if (ok) {
            flash_upgrade();
        }
    } else if (parse_command(&text, "help")) {
        if (parse_word(&text, "about")) {
            ok = ! *text;
            if (ok) {
                print_lines(help_about);
            }
        } else if (! *text) {
            print_lines(help_commands);
        } else {
            ok = false;
        }
    } else {
        ok = false;
    }

    if (! ok) {
        terminal_command_error(text-text_in);
    }
}

// called by the terminal when a command line has been received
static void
main_command_cbfn(char *command)
{
    assert(! main_command);
    main_command = command;
}

// called by the terminal when the user presses Ctrl-C
static void
main_ctrlc_cbfn(void)
{
    stop();
}

// called by the cdcacm transport when the USB device is reset
static void
main_reset_cbfn(void)
{
}

// waits for command lines and runs them
void
main_run(void)
{
    static bool first = true;

    for (;;) {
        terminal_poll();

        if (! main_command) {
            continue;
        }

        // print the echo of the line before what the command prints
        terminal_poll();
        if (terminal_echo) {
            printf("\n");
        }
        run(main_command);

        if (first) {
            print_lines(help_about);
            first = false;
        }
        if (main_prompt) {
            printf("> ");
        } else {
            printf("done\n");
        }

        // a capture command acks its line itself, so Ctrl-C reaches it
        if (main_command) {
            main_command = NULL;
            terminal_command_ack(false);
        }
    }
}

void
main_initialize(void)
{
    terminal_register(main_command_cbfn, main_ctrlc_cbfn);
    cdcacm_register(main_reset_cbfn, terminal_receive);
}
