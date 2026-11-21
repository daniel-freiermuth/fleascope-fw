// *** params.h *******************************************************
// SPDX-License-Identifier: Parity-7.0.0
// persistent parameters: two A/B flash pages at the top of program flash,
// in the layout StickOS used, so devices keep their hostname across
// firmware versions.
//
// page layout (PARAMS_PAGE_SIZE bytes):
//   [0, PARAMS_OFFSET(0))           unused; StickOS kept BASIC flash variables
//                                    here (e.g. probe calibration). Updates
//                                    copy these words along unchanged.
//   [PARAMS_OFFSET(0), ...)          system words, enum param
//   last word                        generation; the higher one is current

#ifndef PARAMS_INCLUDED
#define PARAMS_INCLUDED  1

#include <stdint.h>

#define PARAMS_PAGE_SIZE  4096

// system words; the numbering is the on-flash layout and must not change
enum param {
    PARAM_AUTORUN = 0,
    PARAM_BAUD = 1,
    PARAM_ANALOG = 2,
    PARAM_SERVO = 3,
    PARAM_NODEID = 4,
    PARAM_ASSIGNMENTS_BEGIN = 5,  // 6 pin assignments
    PARAM_WATCH_SMART = 11,
    PARAM_NAME_0 = 12,  // 8 words: hostname, NUL-terminated
    PARAM_NAME_7 = 19,
    PARAM_LAST = PARAMS_PAGE_SIZE/sizeof(uint32_t)/2
};

#define PARAMS_OFFSET(param)  (PARAMS_PAGE_SIZE-sizeof(uint32_t)-(PARAM_LAST-(param))*sizeof(uint32_t))

int32_t
params_get(int param);

void
params_set(int param, int32_t value);

// the hostname, or "Flea-Scope" if none is set
const char *
params_get_name(void);

void
params_set_name(const char *name);

#endif
