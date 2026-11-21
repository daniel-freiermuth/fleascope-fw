// SPDX-License-Identifier: Parity-7.0.0
// Host stand-in for the firmware's main.h, used when firmware modules that
// include "main.h" are compiled for the unit tests (test/host comes first on
// the include path). It provides only what those modules use. Flash lives
// in an array the test owns, so FLASH_START is its address; the sizes come
// from the firmware's config.h.

#ifndef MAIN_INCLUDED
#define MAIN_INCLUDED  1

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t byte;
typedef unsigned int uint;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uintptr_t uintptr;

#define LENGTHOF(a)  (sizeof(a)/sizeof(a[0]))
#define CASSERT(predicate)  _Static_assert(predicate, #predicate)

#include "../../src/config.h"

// the test's simulated program flash, FLASH_BYTES long
extern byte host_flash[];
#undef FLASH_START
#define FLASH_START  ((uintptr)host_flash)

#include "params.h"
#include "../../src/platform/flash.h"

#endif
