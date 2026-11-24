// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for src/app/commands.c (run: make unit-test): the command
// line parser that decides what reaches the capture and signal hardware.
//
// commands.c is included rather than linked so the tests can call its static
// run() on one command line, like main_run() does. Everything it calls
// (scope(), wave(), terminal_command_error(), printf(), ...) is a fake below
// that records its arguments. reset() does not return on the device; the
// fake splx() it calls first jumps back to the test instead.

#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

#include "fleascope-hal.h"
#include "scope-command.h"
#include "stream-command.h"
#include "digital-stream-command.h"
#include "commands.h"

// ---- what the firmware's main.h provides to commands.c --------------------

#define VERSION  "4.0.0"
#define BASIC_OUTPUT_LINE_SIZE  79
#define MIN(a, b)  ((a) < (b) ? (a) : (b))

static uint16_t flash_checksum = 0x2e47;

// reset()'s software reset registers
static volatile uint32_t SYSKEY;
static volatile uint32_t RSWRSTSET;
static volatile uint32_t RSWRST;
#define _RSWRST_SWRST_MASK  1u

// output of printf(), as formatted (the firmware turns '\n' into "\r\n" later)
static char output[4096];
static size_t output_len;

static int
fake_printf(const char *format, ...)
    __attribute__((format(__printf__, 1, 2)));

// pointer differences are int on the PIC32 but long on the host
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#define printf  fake_printf
#include "../../src/app/commands.c"
#undef printf
#pragma GCC diagnostic pop

static int
fake_printf(const char *format, ...)
{
    va_list ap;
    int n;

    va_start(ap, format);
    n = vsnprintf(output+output_len, sizeof(output)-output_len, format, ap);
    va_end(ap);
    TEST_ASSERT_TRUE_MESSAGE(n >= 0 && (size_t)n < sizeof(output)-output_len, "output buffer full");
    output_len += (size_t)n;
    return n;
}

// ---- fakes -----------------------------------------------------------------

bool terminal_echo;

static struct {
    int calls;
    int sample_length;
    TriggerEdge edge;
    TriggerChannel channel;
    int trigger_value;
    int bits_of_interest;
    int delay_samples;
} scope_rec;

static struct {
    int calls;
    uint timer_period_ticks;
    uint averaging;
    uint bitwidth;
} stream_rec;

static struct {
    int calls;
    int channel_mask;
    int us_per_sample;
    int n_samples_per_packet;
} digital_stream_rec;

static struct {
    int calls;
    WaveShape shape;
    int hz;
    bool result;  // what wave() answers
} wave_rec;

static int wave_off_calls;
static int error_calls;
static int error_offset;
static int set_name_calls;
static char set_name[64];
static int upgrade_calls;
static int reset_calls;

static jmp_buf reset_jump;
static bool reset_jump_set;

void
scope(int sample_length, TriggerEdge edge, TriggerChannel channel, int trigger_value, int bits_of_interest, int delay_samples)
{
    scope_rec.calls++;
    scope_rec.sample_length = sample_length;
    scope_rec.edge = edge;
    scope_rec.channel = channel;
    scope_rec.trigger_value = trigger_value;
    scope_rec.bits_of_interest = bits_of_interest;
    scope_rec.delay_samples = delay_samples;
}

void
stream(uint timer_period_ticks, uint averaging, uint bitwidth)
{
    stream_rec.calls++;
    stream_rec.timer_period_ticks = timer_period_ticks;
    stream_rec.averaging = averaging;
    stream_rec.bitwidth = bitwidth;
}

void
digital_stream(int channel_mask, int us_per_sample, int n_samples_per_packet)
{
    digital_stream_rec.calls++;
    digital_stream_rec.channel_mask = channel_mask;
    digital_stream_rec.us_per_sample = us_per_sample;
    digital_stream_rec.n_samples_per_packet = n_samples_per_packet;
}

bool
wave(WaveShape shape, int hz)
{
    wave_rec.calls++;
    wave_rec.shape = shape;
    wave_rec.hz = hz;
    return wave_rec.result;
}

void
wave_off(void)
{
    wave_off_calls++;
}

void
terminal_command_error(int offset)
{
    error_calls++;
    error_offset = offset;
}

const char *
params_get_name(void)
{
    return "Flea-Scope";
}

void
params_set_name(const char *name)
{
    set_name_calls++;
    snprintf(set_name, sizeof(set_name), "%s", name);
}

void
flash_upgrade(void)
{
    upgrade_calls++;
}

// the first thing reset() does; leaves it instead of the endless RSWRST loop
int
splx(int level)
{
    TEST_ASSERT_EQUAL_INT(7, level);
    reset_calls++;
    if (reset_jump_set) {
        longjmp(reset_jump, 1);
    }
    return 0;
}

// called only by main_run()/main_initialize(), which the tests do not run
void
terminal_poll(void)
{
}

void
terminal_command_ack(bool edit)
{
    (void)edit;
}

void
terminal_register(terminal_command_cbfn command_in, terminal_ctrlc_cbfn ctrlc)
{
    (void)command_in;
    (void)ctrlc;
}

bool
terminal_receive(byte *buffer, int length)
{
    (void)buffer;
    (void)length;
    return false;
}

void
cdcacm_register(cdcacm_reset_cbfn reset_cbfn, cdcacm_receive_cbfn receive)
{
    (void)reset_cbfn;
    (void)receive;
}

void
setUp(void)
{
    memset(&scope_rec, 0, sizeof(scope_rec));
    memset(&stream_rec, 0, sizeof(stream_rec));
    memset(&digital_stream_rec, 0, sizeof(digital_stream_rec));
    memset(&wave_rec, 0, sizeof(wave_rec));
    wave_rec.result = true;
    wave_off_calls = 0;
    error_calls = 0;
    error_offset = -1;
    set_name_calls = 0;
    set_name[0] = '\0';
    upgrade_calls = 0;
    reset_calls = 0;
    output[0] = '\0';
    output_len = 0;
    terminal_echo = true;
    main_prompt = true;
}

void
tearDown(void)
{
}

// runs one command line as main_run() does
static void
run_line(const char *line)
{
    if (! setjmp(reset_jump)) {
        reset_jump_set = true;
        run(line);
    }
    reset_jump_set = false;
}

// the line is a syntax error with the caret at offset, and runs nothing
static void
assert_error_at(const char *line, int offset)
{
    run_line(line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, error_calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(offset, error_offset, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, scope_rec.calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, stream_rec.calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, digital_stream_rec.calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, wave_rec.calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, wave_off_calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_name_calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, upgrade_calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, reset_calls, line);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", output, line);
}

static void
assert_error(const char *line)
{
    assert_error_at(line, (int)strlen(line));
}

static void
assert_scope(const char *line, int sample_length, TriggerEdge edge, TriggerChannel channel,
             int trigger_value, int bits_of_interest, int delay_samples)
{
    setUp();
    run_line(line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, error_calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, scope_rec.calls, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(sample_length, scope_rec.sample_length, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(edge, scope_rec.edge, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(channel, scope_rec.channel, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(trigger_value, scope_rec.trigger_value, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(bits_of_interest, scope_rec.bits_of_interest, line);
    TEST_ASSERT_EQUAL_INT_MESSAGE(delay_samples, scope_rec.delay_samples, line);
}

// ---- scope -----------------------------------------------------------------

static void
test_scope_defaults(void)
{
    assert_scope("scope", 10, LEVEL_AKA_MATCHING, ANALOG, -1, -1, 0);
    assert_scope("scope 18000", 18000, LEVEL_AKA_MATCHING, ANALOG, -1, -1, 0);
}

static void
test_scope_trigger_prefix_selects_edge(void)
{
    assert_scope("scope 10 +500", 10, RISE_AKA_START, ANALOG, 500, -1, 0);
    assert_scope("scope 10 -500", 10, FALL_AKA_STOP, ANALOG, 500, -1, 0);
    assert_scope("scope 10 ~500", 10, AUTO, ANALOG, 500, -1, 0);
    assert_scope("scope 10 500", 10, LEVEL_AKA_MATCHING, ANALOG, 500, -1, 0);
}

// a leading 0x selects the digital trigger; the mask may be decimal or hex
static void
test_scope_0x_selects_digital_channel(void)
{
    assert_scope("scope 10 +0x5 0x1ff", 10, RISE_AKA_START, DIGITAL, 5, 0x1ff, 0);
    assert_scope("scope 10 -0x1a 511 20", 10, FALL_AKA_STOP, DIGITAL, 0x1a, 511, 20);
    assert_scope("scope 10 0x0", 10, LEVEL_AKA_MATCHING, DIGITAL, 0, -1, 0);
    assert_scope("scope 10 ~0 0x3", 10, AUTO, ANALOG, 0, 3, 0);
}

static void
test_scope_boundaries(void)
{
    assert_scope("scope 1 1023 1023 1000000", 1, LEVEL_AKA_MATCHING, ANALOG, 1023, 1023, 1000000);
    assert_scope("scope 10 0x3ff 0x3ff 0", 10, LEVEL_AKA_MATCHING, DIGITAL, 0x3ff, 0x3ff, 0);

    setUp();
    assert_error("scope 0");
    setUp();
    assert_error("scope 10 1024");
    setUp();
    assert_error("scope 10 0x400");
    setUp();
    assert_error("scope 10 +5 1024");
    setUp();
    assert_error("scope 10 +5 0 1000001");
}

static void
test_scope_rejects_bad_syntax(void)
{
    // the level must follow the trigger prefix directly
    assert_error_at("scope 10 + 500", 10);
    setUp();
    assert_error_at("scope 10 ++500", 10);
    setUp();
    assert_error_at("scope 10 500 0 0 1", 17);
    setUp();
    assert_error_at("scope 10 500x", 12);
    setUp();
    assert_error_at("scope x", 6);
}

// ---- stream, digital_stream ------------------------------------------------

static void
test_stream_and_digital_stream_dispatch(void)
{
    run_line("stream");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(1, stream_rec.calls);
    TEST_ASSERT_EQUAL_UINT(2333, stream_rec.timer_period_ticks);
    TEST_ASSERT_EQUAL_UINT(1, stream_rec.averaging);
    TEST_ASSERT_EQUAL_UINT(16, stream_rec.bitwidth);

    run_line("digital_stream 0x1ff");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(1, digital_stream_rec.calls);
    TEST_ASSERT_EQUAL_INT(0x1ff, digital_stream_rec.channel_mask);
    TEST_ASSERT_EQUAL_INT(0, digital_stream_rec.us_per_sample);
    TEST_ASSERT_EQUAL_INT(0, digital_stream_rec.n_samples_per_packet);

    // the mask must be written in hex
    setUp();
    assert_error_at("digital_stream 511", 15);
}

// ---- command matching ------------------------------------------------------

// a command name must end at a space or the end of the line
static void
test_command_name_must_be_a_whole_word(void)
{
    static const char *const lines[] = {
        "scopex", "scope2", "verbose", "stream2", "digital_streamx",
        "wavex", "echoes", "prompty", "resetx", "upgrades", "helpme",
        "hostnames", "Scope", "print x",
    };
    size_t i;

    for (i = 0; i < LENGTHOF(lines); i++) {
        setUp();
        assert_error_at(lines[i], 0);
    }
}

static void
test_empty_line_does_nothing(void)
{
    run_line("");
    run_line("   ");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_STRING("", output);
}

// ---- ver, hostname, reset, upgrade, help ------------------------------------

static void
test_ver(void)
{
    run_line("ver");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_STRING("4.0.0 (checksum 0x2e47)\n", output);

    setUp();
    assert_error_at("ver x", 4);
}

static void
test_reset_and_upgrade_run_only_without_arguments(void)
{
    run_line("reset");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(1, reset_calls);

    setUp();
    assert_error_at("reset now", 6);

    setUp();
    run_line("upgrade");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(1, upgrade_calls);

    setUp();
    assert_error_at("upgrade now", 8);
}

static void
test_hostname(void)
{
    run_line("hostname");
    TEST_ASSERT_EQUAL_STRING("Flea-Scope\n", output);
    TEST_ASSERT_EQUAL_INT(0, set_name_calls);

    run_line("hostname  bench scope");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(1, set_name_calls);
    TEST_ASSERT_EQUAL_STRING("bench scope", set_name);
}

static void
test_help(void)
{
    run_line("help about");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_STRING(help_about, output);

    setUp();
    run_line("help");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_STRING(help_commands, output);

    setUp();
    assert_error_at("help scope", 5);
}

// ---- echo, prompt ----------------------------------------------------------

static void
test_mode(const char *name, bool *var)
{
    char line[32];

    *var = true;
    run_line(name);
    TEST_ASSERT_EQUAL_STRING("on\n", output);

    snprintf(line, sizeof(line), "%s off", name);
    run_line(line);
    TEST_ASSERT_FALSE(*var);
    run_line(name);
    TEST_ASSERT_EQUAL_STRING("on\noff\n", output);

    snprintf(line, sizeof(line), "%s on", name);
    run_line(line);
    TEST_ASSERT_TRUE(*var);
    TEST_ASSERT_EQUAL_INT(0, error_calls);

    setUp();
    *var = false;
    snprintf(line, sizeof(line), "%s onx", name);
    assert_error_at(line, (int)strlen(name)+3);
    setUp();
    *var = false;
    snprintf(line, sizeof(line), "%s on x", name);
    assert_error_at(line, (int)strlen(name)+4);
    setUp();
    snprintf(line, sizeof(line), "%s yes", name);
    assert_error_at(line, (int)strlen(name)+1);
}

static void
test_echo(void)
{
    test_mode("echo", &terminal_echo);
}

static void
test_prompt(void)
{
    test_mode("prompt", &main_prompt);
}

// ---- wave ------------------------------------------------------------------

static void
test_wave_shapes(void)
{
    static const struct {
        const char *line;
        WaveShape shape;
    } cases[] = {
        { "wave square 1000", WAVE_SQUARE },
        { "wave ekg 1000", WAVE_EKG },
        { "wave sine 1000", WAVE_SINE },
        { "wave triangle 1000", WAVE_TRIANGLE },
    };
    size_t i;

    for (i = 0; i < LENGTHOF(cases); i++) {
        setUp();
        run_line(cases[i].line);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, error_calls, cases[i].line);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, wave_rec.calls, cases[i].line);
        TEST_ASSERT_EQUAL_INT_MESSAGE(cases[i].shape, wave_rec.shape, cases[i].line);
        TEST_ASSERT_EQUAL_INT_MESSAGE(1000, wave_rec.hz, cases[i].line);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("", output, cases[i].line);
    }
}

static void
test_wave_zero_stops_and_bad_frequency_is_reported(void)
{
    run_line("wave sine 0");
    TEST_ASSERT_EQUAL_INT(1, wave_off_calls);
    TEST_ASSERT_EQUAL_INT(0, wave_rec.calls);

    setUp();
    wave_rec.result = false;
    run_line("wave square 10000001");
    TEST_ASSERT_EQUAL_INT(0, error_calls);
    TEST_ASSERT_EQUAL_INT(10000001, wave_rec.hz);
    TEST_ASSERT_EQUAL_STRING("bad frequency\n", output);
}

static void
test_wave_rejects_bad_syntax(void)
{
    assert_error_at("wave sawtooth 100", 5);
    setUp();
    assert_error_at("wave sine", 9);
    setUp();
    assert_error_at("wave sine 100 x", 14);
}

// ---- error offset ----------------------------------------------------------

// the offset is counted from the start of the line, including leading spaces;
// the terminal adds 2 for the prompt (docs/commands.md: `scope -5` puts the
// caret under the `-`)
static void
test_error_offset_counts_from_line_start(void)
{
    assert_error_at("scope -5", 6);
    setUp();
    assert_error_at("  scope -5", 8);
    setUp();
    assert_error_at("  bogus", 2);
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_scope_defaults);
    RUN_TEST(test_scope_trigger_prefix_selects_edge);
    RUN_TEST(test_scope_0x_selects_digital_channel);
    RUN_TEST(test_scope_boundaries);
    RUN_TEST(test_scope_rejects_bad_syntax);
    RUN_TEST(test_stream_and_digital_stream_dispatch);
    RUN_TEST(test_command_name_must_be_a_whole_word);
    RUN_TEST(test_empty_line_does_nothing);
    RUN_TEST(test_ver);
    RUN_TEST(test_reset_and_upgrade_run_only_without_arguments);
    RUN_TEST(test_hostname);
    RUN_TEST(test_help);
    RUN_TEST(test_echo);
    RUN_TEST(test_prompt);
    RUN_TEST(test_wave_shapes);
    RUN_TEST(test_wave_zero_stops_and_bad_frequency_is_reported);
    RUN_TEST(test_wave_rejects_bad_syntax);
    RUN_TEST(test_error_offset_counts_from_line_start);
    return UNITY_END();
}
