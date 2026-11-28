#define MSPS  18  // approximate target sample rate
#define INTERLEAVE  5  // we interleave results from 5 adcs

// Configure output compare n as an adc trigger on timer 3: initial low,
// continuous high pulses from compare value r to rs. settle runs after the
// module is turned off and before it is reprogrammed.
#define OC_PULSE_T3(n, r, rs, settle) \
    do { \
        OC##n##CONbits.ON = 0; \
        settle; \
        OC##n##CONbits.OCTSEL = 1;  /* timer 3 */ \
        OC##n##CONbits.OCM = 5;  /* initial low, continuous high pulses */ \
        OC##n##R = (r); \
        OC##n##RS = (rs); \
        OC##n##CONbits.ON = 1; \
    } while (0)

typedef enum {
    WAVE_SQUARE,
    WAVE_EKG,
    WAVE_SINE,
    WAVE_TRIANGLE,
    WAVE_COUNT
} WaveShape;

// interrupt handler for the adc dma buffers; not called directly
void
scope_isr(void);

bool wave(WaveShape shape, int hz);
void wave_off(void);
void scope_initialize(void);
void configure_pmd_once(void);
void reset_buffers(void);
uint get_current(void);
short(* get_bits_buffer(void))[2*128*10];
short* get_adc_buffer(void);
void start_T3(void);
void stop_T3(void);
void enable_adc_dma(void);
void disable_adc_dma(void);
void set_capturing_pin(bool capturing);
void set_waiting_pin(bool waiting);
int get_bitbuffer_length(void);
int get_adcbuffer_length(void);
