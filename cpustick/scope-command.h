typedef enum {
    ANALOG,
    DIGITAL
} TriggerChannel;

typedef enum {
    AUTO,
    LEVEL_AKA_MATCHING,
    RISE_AKA_START,
    FALL_AKA_STOP,
} TriggerEdge;

// most negative delay: at most this much history before the trigger. Sampling continues while we
// locate the trigger (up to 2 adc chunks back) and copy the trigger chunk, with up to 2 chunks still
// in flight when we stop, so this must stay well below the 12800 sample ring (half leaves margin).
#define SCOPE_MAX_PRETRIGGER  6400

// delay_samples < 0 reports samples from before the trigger
void scope(int sample_length, TriggerEdge edge, TriggerChannel channel, int trigger_value, int bits_of_interest, int delay_samples);

int ptovbits(int bits);
int vtopbits(int bits);