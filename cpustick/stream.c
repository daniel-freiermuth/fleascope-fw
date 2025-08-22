#include "main.h"

#if ! defined(__32MK0512GPK064__) && ! defined(__32MK0512MCM064__)
#error  only __32MK__ interleaved adc dma is supported for now
#endif

// XXX -- is there any way to use dma counts to get accurate trigger position mid-capture???

// hardware configuration: ra0, ra1, rb0, rb1, ra11 interleaved analog input

// rc10 is wave out from CDAC2 or OC11

#define MSPS  18  // approximate target sample rate
#define SAMPLES  (2000*MSPS/20)  // approximate width of display canvas
#define EXTRA  200  // extra samples in case web page wants to fine-scale
#define INTERLEAVE  5  // we interleave results from 5 adcs

// command line:
// <sampleinterval> [+-=](<voltage>|0x<bits>) [<slew>|0x<mask>] [<delaysamples>]
//          -10    -> 18  Msps -> 3.6  MHz timer with 5x interleave and 10x magnification
//          -5     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 5x magnification
//          -2     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 2x magnification
// number1 = 1     -> 18  Msps -> 3.6  MHz timer with 5x interleave
//           2     -> 9   Msps -> 1.8  MHz timer with 5x interleave
//           5     -> 3.6 Msps -> 0.72 MHz timer with 5x interleave
//           10    -> 1.8 Msps -> 0.36 MHz timer with 5x interleave
//           20    -> 0.9 Msps -> 0.18 MHz timer with 5x interleave
//           50    -> 360 ksps -> 72   kHz timer with 5x interleave
//           ...
//           10000 -> 1.8 ksps -> 0.36 kHz timer with 5x interleave
//           20000 -> 0.9 ksps -> 0.18 kHz timer with 5x interleave

// adcbuffer is an integral dmabuffer size so we don't need to checks for wraps mid-copy
