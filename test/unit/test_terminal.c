// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for src/platform/terminal.c: how command lines that arrive
// in one USB packet are handed to the command loop, and when the CDC OUT
// endpoint is re-armed (cdcacm_command_ack()). While it is not armed the
// device receives nothing from the host, not even Ctrl-C.
//
// terminal.c keeps its state between tests, so every test leaves it idle:
// no line pending, discard off.

#include <string.h>

#include "unity.h"

#include "main.h"  // test/host/main.h

bool cdcacm_attached;
bool cdcacm_active;

static int acks;     // cdcacm_command_ack() calls: the endpoint was re-armed
static int lines;    // command lines delivered
static char line[BASIC_INPUT_LINE_SIZE];
static int ctrlcs;

// the interrupt mask level: the USB isr delivers packets at SPL_USB
static int level;

int
gpl(void)
{
    return level;
}

int
splx(int level_in)
{
    int old = level;

    level = level_in;
    return old;
}

void
led_unknown_progress(void)
{
}

// the host terminal's input line as the echo draws it: the VT100 sequences
// terminal.c sends, with the cursor at col
static char screen[BASIC_INPUT_LINE_SIZE*2];
static int col;

void
cdcacm_print(const byte *buffer, int length)
{
    int i;
    int n;

    i = 0;
    while (i < length) {
        if (buffer[i] != '\033') {
            TEST_ASSERT_LESS_THAN_INT((int)sizeof(screen)-1, col);
            screen[col++] = (char)buffer[i++];
            continue;
        }
        TEST_ASSERT_TRUE(i+2 < length && buffer[i+1] == '[');
        i += 2;
        n = 0;
        while (i < length && isdigit(buffer[i])) {
            n = n*10 + buffer[i++] - '0';
        }
        if (! n) {
            n = 1;  // a missing or zero count is 1
        }
        TEST_ASSERT_TRUE(i < length);
        switch (buffer[i++]) {
            case 'C':
                col += n;
                break;
            case 'D':
                col -= n;
                break;
            case 'K':
                memset(screen+col, 0, sizeof(screen)-(size_t)col);
                break;
            case 'P':
                memmove(screen+col, screen+col+1, sizeof(screen)-(size_t)col-1);
                break;
            default:
                TEST_FAIL_MESSAGE("unexpected escape sequence");
                break;
        }
        TEST_ASSERT_TRUE(col >= 0 && col < (int)sizeof(screen)-1);
    }
}

void
cdcacm_command_ack(void)
{
    acks++;
}

static void
command_cbfn(char *command)
{
    lines++;
    strcpy(line, command);
}

static void
ctrlc_cbfn(void)
{
    ctrlcs++;
}

// one packet from the host, delivered by the USB isr; returns whether the
// endpoint stays armed
static bool
host_sends(const char *text)
{
    byte buffer[64];
    int x;
    bool armed;

    strcpy((char *)buffer, text);
    x = splx(SPL_USB);
    armed = terminal_receive(buffer, (int)strlen(text));
    splx(x);
    return armed;
}

void
setUp(void)
{
    terminal_register(command_cbfn, ctrlc_cbfn);
    terminal_echo = false;
    cdcacm_attached = false;
    cdcacm_active = false;
    acks = 0;
    lines = 0;
    line[0] = '\0';
    ctrlcs = 0;
}

void
tearDown(void)
{
}

// two lines in one packet run one after the other, and the endpoint is
// re-armed only once both have been acked
static void
test_pipelined_lines_run_in_order(void)
{
    TEST_ASSERT_FALSE(host_sends("ver\rhelp\r"));
    TEST_ASSERT_EQUAL_INT(1, lines);
    TEST_ASSERT_EQUAL_STRING("ver", line);

    terminal_command_ack(false);
    TEST_ASSERT_EQUAL_INT(2, lines);
    TEST_ASSERT_EQUAL_STRING("help", line);
    TEST_ASSERT_EQUAL_INT(0, acks);

    terminal_command_ack(false);
    TEST_ASSERT_EQUAL_INT(2, lines);
    TEST_ASSERT_EQUAL_INT(1, acks);
}

// a capture command (scope, stream, digital_stream) acks its line when it
// starts; the line pipelined after it is discarded and the endpoint is
// re-armed, so the host's Ctrl-C reaches the capture
static void
test_capture_rearms_endpoint_and_discards_pipelined_line(void)
{
    TEST_ASSERT_FALSE(host_sends("scope 10 +512\recho off\r"));
    TEST_ASSERT_EQUAL_INT(1, lines);
    TEST_ASSERT_EQUAL_STRING("scope 10 +512", line);

    // the capture starts, as scope(), stream() and digital_stream() do
    terminal_command_ack_discard();
    TEST_ASSERT_EQUAL_INT(1, lines);
    TEST_ASSERT_EQUAL_INT(1, acks);

    TEST_ASSERT_TRUE(host_sends("\003"));
    TEST_ASSERT_EQUAL_INT(1, ctrlcs);

    // the capture ends; the discarded line does not come back later
    terminal_command_discard(false);
    TEST_ASSERT_FALSE(host_sends("ver\r"));
    TEST_ASSERT_EQUAL_INT(2, lines);
    TEST_ASSERT_EQUAL_STRING("ver", line);
    terminal_command_ack(false);
    TEST_ASSERT_EQUAL_INT(2, lines);
    TEST_ASSERT_EQUAL_INT(2, acks);
}

// echo from here on draws on screen, which starts as an empty input line
static void
start_echoing(void)
{
    // an empty input line, whatever earlier tests left (echo is off)
    TEST_ASSERT_TRUE(host_sends("\025"));
    terminal_poll();
    terminal_echo = true;
    cdcacm_attached = true;
    cdcacm_active = true;
    memset(screen, 0, sizeof(screen));
    col = 0;
}

// one packet from the host, then the main loop echoes it
static void
host_types(const char *text)
{
    TEST_ASSERT_TRUE(host_sends(text));
    terminal_poll();
}

// Enter: the line the terminal shows is the one that runs; the command's
// prompt starts a new, empty input line
static void
host_enters(const char *expected)
{
    TEST_ASSERT_FALSE(host_sends("\r"));
    terminal_poll();
    TEST_ASSERT_EQUAL_STRING(expected, line);
    TEST_ASSERT_EQUAL_STRING(expected, screen);
    terminal_command_ack(false);
    memset(screen, 0, sizeof(screen));
    col = 0;
}

// history recalls in one packet (keys the host buffered while the endpoint
// was not armed) queue more echo than the echo buffer holds: the terminal
// still shows the recalled line, and what is typed next edits that line
static void
test_history_recalls_that_overflow_echo_show_the_recalled_line(void)
{
    static const char l1[] = "hostname aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    static const char l2[] = "hostname bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    static const char l3[] = "hostname ccccccccccccccccccccccccccccccccccccccccccccccccccc";

    start_echoing();
    host_types(l1);
    host_enters(l1);
    host_types(l2);
    host_enters(l2);
    host_types(l3);
    host_enters(l3);

    // Up (l3), Up (l2), Up (l1): 63 + 68 + 68 bytes of echo
    host_types("\033[A\033[A\033[A");
    TEST_ASSERT_EQUAL_STRING(l1, screen);
    TEST_ASSERT_EQUAL_INT(60, col);

    host_types("x");
    host_enters("hostname aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaax");
}

// 30 Backspaces in one packet queue 180 bytes of echo: the terminal shows
// the shortened line with the cursor at its end
static void
test_backspaces_that_overflow_echo_show_the_shortened_line(void)
{
    static const char typed[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWX";
    char bs[31];

    start_echoing();
    host_types(typed);
    memset(bs, '\010', 30);
    bs[30] = '\0';
    host_types(bs);
    TEST_ASSERT_EQUAL_STRING("0123456789abcdefghijklmnopqrst", screen);
    TEST_ASSERT_EQUAL_INT(30, col);
    host_enters("0123456789abcdefghijklmnopqrst");
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pipelined_lines_run_in_order);
    RUN_TEST(test_capture_rearms_endpoint_and_discards_pipelined_line);
    RUN_TEST(test_history_recalls_that_overflow_echo_show_the_recalled_line);
    RUN_TEST(test_backspaces_that_overflow_echo_show_the_shortened_line);
    return UNITY_END();
}
