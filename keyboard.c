#include <plib.h>
#include "port_expander_brl4.h"
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
////////////////////////////////////
#include <stdbool.h>
#include <stdlib.h>
#include <stdfix.h>
#include <string.h>
////////////////////////////////////

#include "keyboard.h"
#include "common.h"

#define N_COLS 5

// Ordered array of column bits
const unsigned int col_pins_map[N_COLS] = {BIT_3, BIT_7, BIT_8, BIT_10, BIT_13};

// All column bits ORed together
const unsigned int col_pins = BIT_3 | BIT_7 | BIT_8 | BIT_10 | BIT_13;

// Maps from [column][row] to key number (0-31).
const char key_map[N_COLS][8] = {
    {-1,  0,  1,  2,  4,  3,  5,  6},
    {23, 24, 25, 26, 28, 27, 29, 30},
    {15, 16, 17, 18, 20, 19, 21, 22},
    {31, -1, -1, -1, -1, -1, -1, -1},
    { 7,  8,  9, 10, 12, 11, 13, 14},
};

// Size of the input buffer
#define BUFFER_SIZE         10

// At least [BUFFER_SIZE * BUFFER_THRESHOLD] inputs in the buffer
// need to be high for the key to be high.
#define BUFFER_THRESHOLD    0.4

// Keeps a buffer of inputs for debouncing purposes.
struct poll_buffer {
    unsigned char pos;
    unsigned char contents[BUFFER_SIZE];
} poll_buffer[N_COLS];

// Current keys pressed
keys_t keys_pressed;

// Current keys waiting for release
keys_t waiting_for_release;

void keyboard_init() {
    // Set up columns
    TRISBSET = col_pins;    // Set as input pins
    CNCONBSET = 1 << 15;
    CNPUBSET = col_pins;    // We want pull up resistors because signal is active low
    PORTB;
    
    // Set up rows
    start_spi2_critical_section;
    mPortYEnablePullUp(-1); // We want pull up resistors because signal is active low
    mPortYSetPinsIn(-1);    // Set as input pins
    end_spi2_critical_section;
}

// Converts an GPIO column input to column number
static inline unsigned int bit_to_col_int(unsigned int x) {
    int i;
    for (i = 0; i < sizeof(col_pins_map) / sizeof(*col_pins_map); i++) {
        if (x == (col_pins & ~col_pins_map[i])) return i;
    }
    return -1;
}

keys_t keyboard_poll() {
    unsigned int col;
    unsigned char rows;
    
    // Read inputs until valid column detected
    start_spi2_critical_section;
    do {
        rows = readPE(GPIOY);
        col = bit_to_col_int(PORTB & col_pins);
    } while (col == -1);
    end_spi2_critical_section;

    // Add row readings to buffer
    poll_buffer[col].contents[poll_buffer[col].pos] = ~rows;
    if (++poll_buffer[col].pos >= BUFFER_SIZE) poll_buffer[col].pos = 0;
    
    // Inspect each of the 8 keys in the detected column
    int i;
    for (i = 0; i < 8; i++) {
        // Count highs from the corresponding bit in the buffer
        unsigned int pos, count = 0;
        for (pos = 0; pos < BUFFER_SIZE; pos++) {
            if (poll_buffer[col].contents[pos] & (1 << i)) count++;
        }
        
        unsigned char key = key_map[col][i];                    // Key (0-31)
        bool state = count >= BUFFER_SIZE * BUFFER_THRESHOLD;   // High or low
        
        if (key >= 32 && state) {
            // Bad state: unassigned button turned on (this shouldn't happen)
            tft_fillScreen(ILI9340_RED);
        } else if (key < 32) {
            bool key_waiting_for_release = waiting_for_release & (1 << key);
            
            if (state && !key_waiting_for_release) {
                // If the state of the key is high and it's
                // not waiting for release, then it is pressed
                keys_pressed |= 1 << key;
            } else if (!state) {
                // It is not pressed, nor is it waiting for release anymore
                keys_pressed &= ~(1 << key);
                waiting_for_release &= ~(1 << key);
            }
        }
    }
    
    // Return current keys pressed
    return keys_pressed;
}

void keyboard_wait_for_release() {
    // Clear all keys pressed state
    keys_pressed = 0;
    // Wait for all keys to release
    waiting_for_release = -1;
}
