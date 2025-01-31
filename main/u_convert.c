#include <stdint.h>
#include "settings.h"
#include "u_convert.h"

int32_t steps_to_pulses(int32_t steps) {
    return steps * settings.units.pulses_rev / settings.units.steps_rev;
}

int32_t pulses_to_steps(int32_t pulses) {
    return (pulses * (int32_t)settings.units.steps_rev / (int32_t)settings.units.pulses_rev);
}

uint32_t steps_to_pulses_u(uint32_t steps) {
    return steps * settings.units.pulses_rev / settings.units.steps_rev;
}

uint32_t pulses_to_steps_u(uint32_t pulses) {
    return pulses * settings.units.steps_rev / settings.units.pulses_rev;
}
//float steps_to_mm(uint32_t steps) {
//    return (float)steps * settings.units.mm_per_rev / settings.units.steps_per_rev;
//}

// uint32_t mm_to_steps(float mm) {
//     return (uint32_t)(mm);
// }

float pulses_to_mm(int32_t pulses) {
    return pulses * (int32_t)settings.units.mm_rev / (float)settings.units.pulses_rev;
}

uint32_t mm_to_pulses(uint32_t mm) {
    return mm * settings.units.pulses_rev / settings.units.mm_rev;
}

uint32_t mm_to_pulses_f(float mm) {
    return (uint32_t)((uint32_t)mm * settings.units.pulses_rev / settings.units.mm_rev);
}

uint32_t pulses_to_microm(int32_t pulses) {
    return (uint32_t)pulses * settings.units.mm_rev / settings.units.pulses_rev * (uint32_t)1000;
}