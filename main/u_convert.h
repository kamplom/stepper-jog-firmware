#include <stdint.h>

int32_t steps_to_pulses(int32_t steps);
int32_t pulses_to_steps(int32_t pulses);
uint32_t steps_to_pulses_u(uint32_t steps);
uint32_t pulses_to_steps_u(uint32_t pulses); 
//float steps_to_mm(uint32_t steps);
//uint32_t mm_to_steps(float mm);
float pulses_to_mm(int32_t pulses);
uint32_t mm_to_pulses(uint32_t mm);
uint32_t mm_to_pulses_f(float mm);
