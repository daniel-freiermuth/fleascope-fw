// *** main.h *********************************************************

#ifndef MAIN_INCLUDED

// Versioning follows Semantic Versioning (semver.org):
//   MAJOR — breaking change to the serial wire protocol:
//            command removed, argument added as required, encoding changed
//   MINOR — backwards-compatible addition:
//            new optional command or argument with a defined default
//   PATCH — bug fix, no protocol change
#define VERSION  "4.0.0"

// debug assert()s and checks: `make SODEBUG=1`
#ifndef SODEBUG
#define SODEBUG  0
#endif
#define DEBUGGING  1  // enable to use mplab x debugger

// Enable in-memory trace buffer for debugging with the trace() macro.
#define IN_MEMORY_TRACE 0

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef intptr_t intptr;
typedef uintptr_t uintptr;

#include <myplib.h>

#define asm_halt()  asm("SDBBP 0")

typedef unsigned char byte;
typedef unsigned int uint;

#define IN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define LENGTHOF(a)  (sizeof(a)/sizeof((a)[0]))
#define IS_POWER_OF_2(x) ((((x)-1)&(x))==0)

#define BASIC_OUTPUT_LINE_SIZE  79
#define BASIC_INPUT_LINE_SIZE  72

#define vsnprintf  myvsnprintf
#define snprintf  mysnprintf
#define sprintf  mysprintf
#define vsprintf  myvsprintf
#define printf  myprintf
#define sprintf_s mysprintf_s
#define snprintf_s mysnprintf_s

#include <stdarg.h>

#include "flash.h"
#include "pin.h"
#include "printf.h"
#include "terminal.h"
#include "timer.h"
#include "util.h"
#include "adc.h"
#include "led.h"
#include "params.h"

#include "config.h"

#include "cdcacm.h"
#include "usb.h"

#include "commands.h"
#include "fleascope-hal.h"
#include "scope-command.h"
#include "stream-command.h"
#include "digital-stream-command.h"

#define os_yield()  // NULL

#define CASSERT(predicate) _impl_CASSERT_LINE(predicate,__LINE__)
#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line) \
    typedef char _impl_PASTE(cassert_failed_line##_,line)[2*!!(predicate)-1]

// usable in function body (a local typedef would be an unused-local-typedef)
#define cassert(predicate)  do { _Static_assert(predicate, #predicate); } while (0)

// debug_only(statement) runs statement only in SODEBUG builds, e.g. assert() bookkeeping
#if SODEBUG
#define assert(x)  do { if (! (x)) { led_line(__LINE__); } } while (0)
#define debug_only(x)  do { (x); } while (0)
#else
#define assert(x)
#define debug_only(x)
#endif
#define assert_ram(x)  do { if (! (x)) { asm_halt(); } } while (0)
#define ASSERT(x)  do { if (! (x)) { led_line(__LINE__); } } while (0)

extern uint16 flash_checksum;

extern byte *end_of_static;

extern uint32 bus_frequency;

extern bool debugger_attached;

// exception handlers, called by the XC32 startup code
void
_general_exception_context(void);  // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp): name required by XC32 startup code

void
_bootstrap_exception_handler(void);  // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp): name required by XC32 startup code

typedef void (*flash_upgrade_ram_begin_f)(void);

void
__longramfunc__
__attribute__((nomips16))
flash_upgrade_ram_begin(void);

void
flash_upgrade_ram_end(void);

#define MAIN_INCLUDED  1
#endif  // MAIN_INCLUDED

