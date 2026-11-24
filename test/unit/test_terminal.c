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

void
cdcacm_print(const byte *buffer, int length)
{
    (void)buffer;
    (void)length;
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

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pipelined_lines_run_in_order);
    RUN_TEST(test_capture_rearms_endpoint_and_discards_pipelined_line);
    return UNITY_END();
}
