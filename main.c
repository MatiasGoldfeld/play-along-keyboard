////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
#include "pt_cornell_1_3_2.h"
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// port expander
#include "port_expander_brl4.h"
////////////////////////////////////
#include <stdbool.h>
#include <stdlib.h>
#include <stdfix.h>
#include <string.h>
////////////////////////////////////
#include "common.h"
#include "leds.h"
#include "song.h"
#include "player.h"
#include "keyboard.h"
////////////////////////////////////

enum state {
    MENU,       // Selecting from songs or freeplay
    PLAYING,    // Playing a song
    FREEPLAY    // Freeplay (LEDs match inputs)
};

// Draws the [i]th menu item ([i]th song), [highlighted] if selected.
void menu_draw_item(unsigned int i, bool highlighted) {
    tft_setTextSize(2);
    if (highlighted) {
        tft_setTextColor2(ILI9340_BLUE, ILI9340_WHITE);
    } else {
        tft_setTextColor2(ILI9340_WHITE, ILI9340_BLACK);
    }
    tft_setCursor(0, 0 + 20 * i);
    tft_writeString(songs[i].title);
}

// Runs the menu. Returns the next state.
enum state menu_loop() {
    static int menu_pos = 0;
    static bool redraw_required = true; // Redraws entire menu when [true]
    
    keys_t keys = keyboard_poll();
    bool up = keys & (1 << 0);      // Up:      first  white key
    bool down = keys & (1 << 2);    // Down:    second white key
    bool select = keys & (1 << 4);  // Select:  third  white key
    
    if (up || down || select) {
        // If any button is pressed, wait for release
        // to prevent multiple actions from occurring rapidly
        keyboard_wait_for_release();
    }
    
    if (select) {
        redraw_required = true; // Will need to redraw menu next time
        if (menu_pos == n_songs - 1) {
            // Freeplay mode selected
            tft_fillScreen(ILI9340_BLACK);
            tft_fillTriangle(0, 0, tft_width(), 0, tft_width() / 2, tft_height(), ILI9340_WHITE);
            return FREEPLAY;
        } else {
            // Song number [menu_pos] selected
            player_start_song(&songs[menu_pos]);
            return PLAYING;
        }
    }
    
    // Handle going up and down the menu
    int new_menu_pos = menu_pos - (up ? 1 : 0) + (down ? 1 : 0);    
    new_menu_pos = max(0, min(n_songs - 1, new_menu_pos));
    
    // Remove old highlight and draw new highlight if no redraw required
    if (new_menu_pos != menu_pos && !redraw_required) {
        menu_draw_item(menu_pos, false);
        menu_draw_item(new_menu_pos, true);
    }
    menu_pos = new_menu_pos;
    
    // Redraw entire menu if required
    if (redraw_required) {
        redraw_required = false;
        tft_fillScreen(ILI9340_BLACK);
        
        int i;
        for (i = 0; i < n_songs; i++) {
            menu_draw_item(i, i == menu_pos);
        }
    }
    
    // Nothing selected, so still in menu
    return MENU;
}

static PT_THREAD (tft_test(struct pt *pt)) {
    PT_BEGIN(pt);
    
    static enum state state = MENU;
    
    while (true) {
        switch (state) {
            case MENU:
                state = menu_loop();
                break;
            case PLAYING:
                if (player_update()) {
                    // Song has finished, back to menu
                    state = MENU;
                    // Small delay so it doesn't boot you out to the menu quick
                    delay_ms(3000);
                }
                break;
            case FREEPLAY:
                // Love how nicely this turned out :)
                // Sets LEDs to keyboard input
                leds_set(keyboard_poll());
                break;
        }
    }   
    PT_END(pt);  
}

// === Main  ======================================================
void main(void) { 
    ANSELA = 0; ANSELB = 0; 

    // === config threads ==========
    // turns OFF UART support and debugger pin, unless defines are set
    PT_setup();

    // === setup system wide interrupts  ========
    INTEnableSystemMultiVectoredInt();

    // Initialize the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    tft_setRotation(1); // Set up 320x240 horizontal mode

    // Initialize the threads
    pt_add(tft_test, 0);
    
    // static seed
    srand(1);
    
    // Initialize the port expander
    initPE();
    
    // Initialize keyboard LEDs
    leds_init();

    // Initialize keyboard input
    keyboard_init();

    // === Initalize the scheduler ====================
    PT_INIT(&pt_sched) ;
    pt_sched_method = SCHED_ROUND_ROBIN ;
  
    // === scheduler thread =======================
    // scheduler never exits
    PT_SCHEDULE(protothread_sched(&pt_sched));
    // ============================================
}
