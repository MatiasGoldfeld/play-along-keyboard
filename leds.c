#include <plib.h>
#include "port_expander_brl4.h"
////////////////////////////////////
#include <stdbool.h>
#include <stdlib.h>
#include <stdfix.h>
#include <string.h>
////////////////////////////////////

#include "leds.h"

#define SRCLK BIT_5 // Shift   register clock pin
#define RCLK  BIT_4 // Storage register clock pin

// Which pins are assigned to each of the four shift registers
//                                    00-07, 08-15, 16-23, 24-31
const unsigned char serial_pins[4] = {BIT_0, BIT_1, BIT_2, BIT_3};

// Current state of the LEDs
// The shift registers by default have them on, so this starts them all on too
keys_t leds_state = -1;

void leds_init() {
    unsigned char pins = SRCLK | RCLK;
    int i;
    for (i = 0; i < sizeof(serial_pins) / sizeof(*serial_pins); i++) {
        pins |= serial_pins[i];
    }
    
    // Set up port Z on the port expander and clear its output just in case
    start_spi2_critical_section;
    mPortZSetPinsOut(pins);
    writePE(GPIOZ, 0);
    end_spi2_critical_section;
    
    // Turn all the LEDs off
    leds_set(0);
}

void leds_update() {
    int i;
    start_spi2_critical_section;
    // Sending each of the 8 bits of the shift registers in seral
    for (i = 0; i < 8; i++) {
        unsigned char active_pins = 0;
        
        // For each shift register, figure out if we want its data pin active
        int pin;
        for (pin = 0; pin < sizeof(serial_pins) / sizeof(*serial_pins); pin++) {
            // We subtract from 31 because the LEDs are reversed in hardware
            int led = 31 - 8 * pin - i;
            // uhhhh... Marek switched LEDs 13 and 14...
            // Remove the following line if you wired your LEDs correctly.
            if (led == 13) led = 14;
            else if (led == 14) led = 13;
            if (leds_state & (1 << led)) {
                active_pins |= serial_pins[pin];
            }
        }

        // Shift register send data and shift routine
        // The delays could probably be reduced more from 100us
        writePE(GPIOZ, active_pins);
        delay_us(100);
        writePE(GPIOZ, active_pins | SRCLK);
        delay_us(100);
        writePE(GPIOZ, active_pins);    // Maybe don't need this one?
        delay_us(100);
        writePE(GPIOZ, 0);
    }
    
    // Shift register store routine
    delay_us(100);
    writePE(GPIOZ, RCLK);
    delay_us(100);
    writePE(GPIOZ, 0);
    end_spi2_critical_section;
}

void leds_set(keys_t keys) {
    // If there's no change, then don't bother updating the LEDs
    if (keys != leds_state) {
        leds_state = keys;
        leds_update();
    }
}
