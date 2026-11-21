// SPDX-License-Identifier: Parity-7.0.0
// Host unit tests for cpustick/flash-vars.c, the StickOS-compatible flash
// variables that hosts use for probe calibration.
//
// What must not change: which parameter-page word a host's variable lands
// on (declaration order since reset), and the replies hosts parse.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "params.h"
#include "flash-vars.h"

// fake parameter page: user words only, erased flash reads -1
static int32_t user[PARAMS_USER_BYTES/sizeof(int32_t)];

int32_t
params_get_user(uint32_t offset)
{
    TEST_ASSERT_EQUAL_UINT32(0, offset%4);
    TEST_ASSERT_LESS_THAN_UINT32(PARAMS_USER_BYTES, offset);
    return user[offset/4];
}

void
params_set_user(uint32_t offset, int32_t value)
{
    TEST_ASSERT_EQUAL_UINT32(0, offset%4);
    TEST_ASSERT_LESS_THAN_UINT32(PARAMS_USER_BYTES, offset);
    user[offset/4] = value;
}

static char reply[256];

// runs line, expecting it to succeed with exactly the given reply
static void
ok(const char *line, const char *expected)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(FLASHVARS_OK, flashvars_run(line, reply, sizeof(reply)), line);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, reply, line);
}

void
setUp(void)
{
    memset(user, 0xff, sizeof(user));
    flashvars_reset();
}

void
tearDown(void)
{
}

// fleascope-rs: read_calibration_from_flash() for x1, then x10, after a reset
static void
test_fleascope_rs_layout(void)
{
    user[0] = 1234;
    user[1] = 2345;
    user[2] = 1111;
    user[3] = 2222;

    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash", "");
    ok("print cal_zero_x1", "1234\n");
    ok("print cal_3v3_x1", "2345\n");
    ok("dim cal_zero_x10 as flash, cal_3v3_x10 as flash", "");
    ok("print cal_zero_x10", "1111\n");
    ok("print cal_3v3_x10", "2222\n");
}

// a second connection without reset sees its variables already declared,
// one message per declaration, as fleascope-rs expects
static void
test_redeclare_reports_each(void)
{
    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash", "");
    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash",
       "var 'cal_zero_x1' already declared at this scope\n"
       "var 'cal_3v3_x1' already declared at this scope\n");
}

// python/scope.py and the original web client reserve unused[8] first
static void
test_python_layout(void)
{
    user[8] = 1001;
    user[9] = 1002;
    user[10] = 1003;
    user[11] = 1004;

    ok("dim unused[8] as flash", "");
    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash", "");
    ok("dim cal_zero_x10 as flash, cal_3v3_x10 as flash", "");
    ok("print cal_zero_x1, cal_3v3_x1, cal_zero_x10, cal_3v3_x10", "1001 1002 1003 1004\n");
}

// write_calibration_to_flash(), then a reset and the same declarations
static void
test_assignment_persists_across_reset(void)
{
    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash", "");
    ok("cal_zero_x1 = 1050", "");
    ok("let cal_3v3_x1 = -7", "");
    TEST_ASSERT_EQUAL_INT32(1050, user[0]);
    TEST_ASSERT_EQUAL_INT32(-7, user[1]);

    flashvars_reset();
    ok("print cal_zero_x1", "var 'cal_zero_x1' undefined\n\n");
    ok("dim cal_zero_x1 as flash, cal_3v3_x1 as flash", "");
    ok("print cal_zero_x1, cal_3v3_x1", "1050 -7\n");
}

static void
test_arrays_and_hex(void)
{
    ok("dim a[3] as flash, b as flash", "");
    ok("a[2] = 0x1f, b = 5", "");
    TEST_ASSERT_EQUAL_INT32(31, user[2]);
    TEST_ASSERT_EQUAL_INT32(5, user[3]);
    ok("print a[2], b", "31 5\n");
    ok("print a[3]", "var 'a' index 3 out of range\n0\n");
    ok("dim z[0] as flash", "declared 0 length array\n");
}

// expected replies come from StickOS 3.1.0's interpreter (make guest at
// 5e344e3): an error message comes first and the statement goes on
static void
test_runtime_errors_continue_like_stickos(void)
{
    ok("dim a[3] as flash, b as flash", "");
    ok("a = 0, a[1] = 5, a[2] = 0, b = 0", "");  // as after a reflash
    ok("print b, nope", "var 'nope' undefined\n0 \n");
    ok("print nope, b", "var 'nope' undefined\n 0\n");
    ok("print a[3], b", "var 'a' index 3 out of range\n0 0\n");
    ok("print b[1]", "var 'b' index 1 out of range\n0\n");
    ok("print a", "0 5 0\n");
    ok("print a, b", "0 5 0 0\n");
    ok("print a[-1]", "0\n");
    ok("nope = 3, b = 4", "var 'nope' undefined\n");
    ok("b = 7, nope = 3", "var 'nope' undefined\n");
    ok("print b", "7\n");
}

static void
test_out_of_parameter_flash(void)
{
    char line[64];

    snprintf(line, sizeof(line), "dim big[%u] as flash", (unsigned)(PARAMS_USER_BYTES/4));
    ok(line, "");
    ok("dim one as flash", "out of parameter flash\n");
}

// the whole line is parsed before anything runs
static void
test_syntax_error_has_no_effect(void)
{
    // the offset points at the offending character
    TEST_ASSERT_EQUAL_INT(16, flashvars_run("dim a as flash, 3", reply, sizeof(reply)));
    ok("print a", "var 'a' undefined\n\n");
    TEST_ASSERT_EQUAL_INT(9, flashvars_run("dim a as ram", reply, sizeof(reply)));
    TEST_ASSERT_EQUAL_INT(8, flashvars_run("print a b", reply, sizeof(reply)));
}

// other command lines are left to the command dispatcher
static void
test_not_mine(void)
{
    TEST_ASSERT_EQUAL_INT(FLASHVARS_NOT_MINE, flashvars_run("scope 10 ~512", reply, sizeof(reply)));
    TEST_ASSERT_EQUAL_INT(FLASHVARS_NOT_MINE, flashvars_run("frobnicate", reply, sizeof(reply)));
}

int
main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fleascope_rs_layout);
    RUN_TEST(test_redeclare_reports_each);
    RUN_TEST(test_python_layout);
    RUN_TEST(test_assignment_persists_across_reset);
    RUN_TEST(test_arrays_and_hex);
    RUN_TEST(test_runtime_errors_continue_like_stickos);
    RUN_TEST(test_out_of_parameter_flash);
    RUN_TEST(test_syntax_error_has_no_effect);
    RUN_TEST(test_not_mine);
    return UNITY_END();
}
