// *** params.c *******************************************************
// SPDX-License-Identifier: Parity-7.0.0
// persistent parameters stored in two A/B flash pages; see params.h.

#include "main.h"

CASSERT(PARAMS_PAGE_SIZE == FLASH_PAGE_SIZE);

#define PARAMS_WORDS  (PARAMS_PAGE_SIZE/sizeof(uint32))
#define PAGE_A  ((uint32 *)(FLASH_START+FLASH_BYTES-2*PARAMS_PAGE_SIZE))
#define PAGE_B  ((uint32 *)(FLASH_START+FLASH_BYTES-PARAMS_PAGE_SIZE))

static uint32 scratch[PARAMS_WORDS];

// generation + 1 in int32 wraparound arithmetic; erased flash counts as 0
static int32
rank(const uint32 *page)
{
    return (int32)(page[PARAMS_WORDS-1]+1);
}

static uint32 *
current(void)
{
    return rank(PAGE_A) > rank(PAGE_B) ? PAGE_A : PAGE_B;
}

static uint32 *
other(void)
{
    return current() == PAGE_A ? PAGE_B : PAGE_A;
}

// copy the current page into the other one with nwords words replaced at
// byte offset offset, then bump the generation
static void
update(uint32 offset, const uint32 *words, uint32 nwords)
{
    uint32 *cur;
    uint32 *next;
    uint32 generation;

    assert(! (offset%sizeof(uint32)));
    assert(offset/sizeof(uint32)+nwords <= PARAMS_WORDS-1);

    cur = current();
    next = other();
    generation = cur[PARAMS_WORDS-1];

    memcpy(scratch, cur, PARAMS_PAGE_SIZE-sizeof(uint32));
    memcpy(scratch+offset/sizeof(uint32), words, nwords*sizeof(uint32));

    flash_erase_pages(next, 1);
    flash_write_words(next, scratch, PARAMS_WORDS-1);
    generation++;
    flash_write_words(next+PARAMS_WORDS-1, &generation, 1);
}

// The first boot after a reflash or `upgrade` (which erases the upper half
// of flash, these pages included) finds no page ever written. StickOS then
// cleared its flash variables to 0, and hosts rely on that: the original web
// GUI takes a calibration word of 0 as "not calibrated". Do the same, keeping
// the system words as they are.
void
params_initialize(void)
{
    uint32 *cur;
    uint32 *next;
    uint32 generation;

    cur = current();
    if (cur[PARAMS_WORDS-1] != 0xFFFFFFFF) {
        return;
    }
    next = other();

    memcpy(scratch, cur, PARAMS_PAGE_SIZE-sizeof(uint32));
    memset(scratch, 0, PARAMS_USER_BYTES);

    flash_erase_pages(next, 1);
    flash_write_words(next, scratch, PARAMS_WORDS-1);
    generation = 0;
    flash_write_words(next+PARAMS_WORDS-1, &generation, 1);
}

int32
params_get(int param)
{
    assert(param >= 0 && param < PARAM_LAST);
    return (int32)current()[PARAMS_OFFSET(param)/sizeof(uint32)];
}

void
params_set(int param, int32 value)
{
    uint32 word;

    assert(param >= 0 && param < PARAM_LAST);
    if (params_get(param) == value) {
        return;
    }
    word = (uint32)value;
    update(PARAMS_OFFSET(param), &word, 1);
}

const char *
params_get_name(void)
{
    const uint32 *name;

    name = current()+PARAMS_OFFSET(PARAM_NAME_0)/sizeof(uint32);
    if ((int32)name[0] == -1 || ! *(const char *)name) {
        return "Flea-Scope";
    }
    return (const char *)name;
}

void
params_set_name(const char *name)
{
    uint32 words[PARAM_NAME_7-PARAM_NAME_0+1];
    size_t n;

    assert(name);
    memset(words, 0, sizeof(words));
    n = strlen(name);
    if (n > sizeof(words)-1) {
        n = sizeof(words)-1;
    }
    memcpy(words, name, n);
    update(PARAMS_OFFSET(PARAM_NAME_0), words, LENGTHOF(words));
}

int32
params_get_user(uint32 offset)
{
    assert(! (offset%sizeof(uint32)) && offset < PARAMS_USER_BYTES);
    return (int32)current()[offset/sizeof(uint32)];
}

void
params_set_user(uint32 offset, int32 value)
{
    uint32 word;

    assert(! (offset%sizeof(uint32)) && offset < PARAMS_USER_BYTES);
    if (params_get_user(offset) == value) {
        return;
    }
    word = (uint32)value;
    update(offset, &word, 1);
}
