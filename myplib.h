#include <xc.h>
#include <sys/kmem.h>
#include <sys/attribs.h>

unsigned int __attribute__((nomips16))  INTEnableInterrupts(void);
void __attribute__ ((nomips16)) INTEnableSystemMultiVectoredInt(void);


#undef __ISR
#define __ISR(v,ipl) __attribute__((vector(v), interrupt(ipl), micromips))
#define EXCEPTION_HANDLER  __attribute__((micromips))





#define ReadADC10(bufIndex) ((uint16_t) (*((&ADCDATA12) + (bufIndex << 2))))



