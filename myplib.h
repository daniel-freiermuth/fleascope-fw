#include <xc.h>
#include <sys/kmem.h>
#include <sys/attribs.h>

unsigned int __attribute__((nomips16))  INTEnableInterrupts(void);
void __attribute__ ((nomips16)) INTEnableSystemMultiVectoredInt(void);

#undef __ISR
#ifdef __clang__
// clang only parses this code for analysis (make lint); its MIPS interrupt
// attribute takes a string, not XC32's (vector, IPL) pair
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp): XC32's sys/attribs.h name, used by every ISR
#define __ISR(v,ipl) __attribute__((used))
#else
#define __ISR(v,ipl) __attribute__((vector(v), interrupt(ipl), micromips))
#endif
#define EXCEPTION_HANDLER  __attribute__((micromips))

#define ReadADC10(bufIndex) ((uint16_t) (*((&ADCDATA12) + ((bufIndex) << 2))))
