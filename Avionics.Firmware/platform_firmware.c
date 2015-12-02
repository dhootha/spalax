#include "platform.h"
#include "hal.h"

uint32_t platform_get_counter_value() {
    return halGetCounterValue();
}

uint32_t platform_get_counter_frequency() {
    // The base clock frequency of the chip - not sure if this a decent approximation of halGetCounterFrequency
    // However, it is consistent with the data having an update rate of 1kHz
    return halGetCounterFrequency();
}