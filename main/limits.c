#include <stdint.h>
#include <stdio.h>

#include "settings.h"

uint32_t soft_limits_check(uint32_t pos) {
    if (pos < settings.motion.pos.min) {
        return settings.motion.pos.min;
    } else if (pos > settings.motion.pos.max) {
        return settings.motion.pos.max;
    } else {
        return pos;
    }
}