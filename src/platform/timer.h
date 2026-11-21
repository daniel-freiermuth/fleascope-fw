// *** timer.h ********************************************************

enum { ticks_per_msec = 4 }; // tunable

extern volatile int32 ticks;  // incremented by pit0 isr every tick
extern volatile int32 msecs;  // incremented by pit0 isr every millisecond
extern volatile int32 seconds;  // incremented by pit0 isr every second

extern bool volatile timer_in_isr;

// interrupt handler for timer 1; not called directly
void
timer_isr(void);

void
timer_initialize(void);

