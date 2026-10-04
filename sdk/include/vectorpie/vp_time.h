/* vp_time.h — a clock and a sleep that work the same on the PC and the Pi. */
#ifndef VECTORPIE_VP_TIME_H
#define VECTORPIE_VP_TIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Milliseconds from an arbitrary start; never goes backwards. */
uint64_t vp_ticks_ms(void);

/* Sleep at least `ms` milliseconds (nothing if ms <= 0). */
void vp_sleep_ms(int ms);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_TIME_H */
