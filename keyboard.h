#ifndef _KEYBOARD_H
#define _KEYBOARD_H

/* KEYBOARD.H
 * Manages setting up and receiving input from the keyboard.
 */

#include <stdbool.h>
#include <stdint.h>

#include "common.h"

// Initializes the keyboard state and input pins.
void   keyboard_init();

// Polls the keyboard and returns the current keys pressed.
keys_t keyboard_poll();

// Will make all keys wait for release before they can be activated.
void   keyboard_wait_for_release();

#endif
