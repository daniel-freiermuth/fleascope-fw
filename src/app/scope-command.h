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

void scope(int sample_length, TriggerEdge edge, TriggerChannel channel, int trigger_value, int bits_of_interest, int delay_samples);

int ptovbits(int bits);
int vtopbits(int bits);