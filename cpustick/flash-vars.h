// *** flash-vars.h ***************************************************
// the subset of StickOS BASIC that hosts use to keep calibration in flash:
//
//   dim <name>[[<n>]] as flash[, ...]
//   print <name>[[<i>]][, ...]
//   [let] <name>[[<i>]] = <integer>[, ...]
//
// as in StickOS, declarations live in RAM until reset and take the next free
// words of the parameter page in declaration order; the names are not stored.
// So a host that declares the same names in the same order after each reset
// finds its values again. No other code depends on this module; it only
// exists for wire compatibility with existing hosts.
//
// pure C (no SFR access, no I/O), so it is unit tested on the host.

#ifndef FLASH_VARS_INCLUDED
#define FLASH_VARS_INCLUDED  1

#include <stdbool.h>

#define FLASHVARS_NOT_MINE  -2  // not a dim/print/assignment
#define FLASHVARS_OK        -1  // done; reply holds any output

// forgets all declarations, as a reset does.
void
flashvars_reset(void);

// runs one command line. Returns FLASHVARS_NOT_MINE, FLASHVARS_OK, or the
// offset of a syntax error in line. reply receives the output lines, each
// ending in '\n', truncated to reply_size-1 characters.
int
flashvars_run(const char *line, char *reply, int reply_size);

#endif
