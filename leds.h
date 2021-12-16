#ifndef _LEDS_H
#define _LEDS_H

/* LEDS.H
 * Manages setting up and turning the LEDs on and off.
 */

#include <stdbool.h>

#include "common.h"

// Initializes the LED states and output pins.
void leds_init();

// Sets the LED states to match [keys].
void leds_set(keys_t keys);

#endif
