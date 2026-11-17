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

void scope(int sample_length, int magnify, TriggerEdge edge, TriggerChannel channel, int number2, int number3, int number4);

int ptovbits(int bits);
int vtopbits(int bits);