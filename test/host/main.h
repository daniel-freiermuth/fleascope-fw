// SPDX-License-Identifier: Parity-7.0.0
// Host stand-in for the firmware's main.h, used when firmware modules that
// include "main.h" are compiled for the unit tests (test/host comes first on
// the include path). It provides only what those modules use. Flash lives
// in an array the test owns, so FLASH_START is its address; the sizes come
// from the firmware's config.h. The usb driver, timer and interrupt level
// functions declared here are faked by the tests that need them.
//
// Host builds are the debug configuration: SODEBUG code is compiled and
// assert() is <assert.h>'s, so a broken invariant aborts the test.

#ifndef MAIN_INCLUDED
#define MAIN_INCLUDED  1

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SODEBUG  1

typedef uint8_t byte;
typedef unsigned int uint;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uintptr_t uintptr;

#define LENGTHOF(a)  (sizeof(a)/sizeof((a)[0]))
#define CASSERT(predicate)  _Static_assert(predicate, #predicate)
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define ASSERT(x)  assert(x)

#include "../../src/config.h"

// the test's simulated program flash, FLASH_BYTES long
extern byte *const host_flash;
#undef FLASH_START
#define FLASH_START  ((uintptr)host_flash)

#include "params.h"
#include "../../src/platform/flash.h"

// util.h and timer.h, as far as the host-built modules use them
int
gpl(void);

int
splx(int level);

void
delay(int32 ms);

extern volatile int32 msecs;

#include "usb.h"
#include "cdcacm.h"

#endif
