#include <string.h>

#include "tft_gfx.h"
#include "tft_master.h"

#include "common.h"
#include "keyboard.h"
#include "leds.h"
#include "song.h"

#include "player.h"
#include "tft_master.h"

#define BASE_LETTER 'F' // Letter of key 0
#define BASE_OCTAVE 3   // Octave of key 0

const struct song *song;                // Currently playing song
unsigned int quanta_no;                 // Current quanta in song
unsigned int measure_start_quanta_no;   // Starting quanta of current measure

#define SHEET_BASE_LETTER 'E' // Letter of bottom line of the sheet music

static void display_sheet_music() {
    // Lots of modifiable and dynamic layout variables to construct sheet music
    short sheet_base = tft_height() - 70;
    short line_height = 8;
    short sheet_margin = 15;
    short sheet_right = tft_width() - sheet_margin;
    short sheet_width = sheet_right - sheet_margin;
    short sheet_height = 4 * line_height;
    short sheet_top = sheet_base - sheet_height;
    
    // Clear lower part of screen
    tft_fillRect(0, 35, tft_width(), tft_height(), ILI9340_WHITE);
    
    // Draw sheet music lines
    unsigned int i;
    for (i = 0; i < 5; i++) {
        short y = sheet_base - line_height * i;
        tft_drawLine(sheet_margin, y, sheet_right, y, ILI9340_BLACK);
    }
    for (i = 0; i < 3; i ++) {
        short x = sheet_margin + sheet_width * i / 2;
        tft_drawLine(x, sheet_base, x, sheet_top, ILI9340_BLACK);
    }
    
    // Draw all quantas from current to 2 measures ahead
    for (i = measure_start_quanta_no; i < song->num_quantas; i++) {        
        const struct quanta *current_quanta = &song->quantas[i];
        unsigned int delta_time = current_quanta->time - song->quantas[measure_start_quanta_no].time;
        
        // Stop if note is more than 2 measures ahead
        if (delta_time >= 32) break;
        
        // Horizontal position of quanta
        short x = sheet_margin + sheet_width * delta_time / 32 + line_height / 2 + 14;
        short line_x = x + line_height / 2 - 2; // Where the note lines are
        
        // Red if current quanta, else black
        unsigned short color = i == quanta_no ? ILI9340_RED : ILI9340_BLACK;
        
        // Write chord identifier on top of sheet music
        tft_setTextColor(color);
        if (current_quanta->chord_notes != NULL) {
            tft_setTextSize(2);
            tft_setCursor(x - 10, sheet_top - 40);
            tft_writeString(current_quanta->chord);
        }
        
        short top_wing_height = tft_height();
        int j, wings = 0;
        // Draw all notes in current quanta
        for (j = 0; j < current_quanta->num_melody_notes; j++) {
            beat_duration duration = current_quanta->melody_notes[j].duration;
            struct note note = current_quanta->melody_notes[j].note;
            int pos =
                7 * (note.octave - song->base_sheet_octave) + 
                1 * (note.letter - SHEET_BASE_LETTER);

            // Vertical position of note
            short y = sheet_base - pos * line_height / 2;
            
            // Draw extra lines if note is above or below the sheet
            if (pos < -1) {
                int line_pos;
                for (line_pos = -2; line_pos >= pos; line_pos -= 2) {
                    short y = sheet_base - line_pos * line_height / 2;
                    tft_drawLine(x - line_height * 0.8, y, x + line_height * 0.8, y, ILI9340_BLACK);
                }
            } else if (pos > 11) {
                int line_pos;
                for (line_pos = 12; line_pos <= pos; line_pos += 2) {
                    short y = sheet_base - line_pos * line_height / 2;
                    tft_drawLine(x - line_height * 0.8, y, x + line_height * 0.8, y, ILI9340_BLACK);
                }
            }
            
            // Hollow if 2 beats or more, else solid
            if (duration >= 8) {
                tft_drawCircle(x, y, line_height / 2 - 2, color);
                tft_drawCircle(x, y, line_height / 2 - 1, color);
            } else {
                tft_fillCircle(x, y, line_height / 2 - 1, color);
            }
            
            // Draw note vertical line if 2 beats or less
            if (duration < 16) {
                short note_line_height = line_height * 3.25;
                tft_fillRect(line_x, y - note_line_height, 2, note_line_height, color);
                                
                // Record if note has wings and how many
                if (duration <= 2) {
                    wings = max(wings, 3 - duration);
                    top_wing_height = min(top_wing_height, y - note_line_height);
                }
            }
            
            // Draw flat or sharp
            if (note.accidental != 0) {
                tft_setTextSize(1);
                tft_setCursor(x - line_height / 2 - 7, y - line_height / 2 + 1);
                tft_write(note.accidental == 1 ? '#' : 'b');
            }
        }
        
        // If note has wings, draw them
        for (j = 0; j < wings; j++) {
            short wing_length = line_height * 1.2;
            tft_fillRect(line_x, top_wing_height, wing_length, 2, color);
        }
    }
}

// Translates from a [struct note] to a [key_t]
key_t note_to_key(const struct note *note) {
    /* This is needed to adjust for the fact that music people are weird and
     * B = Cb, B# = C, E = Fb, E# = F on keyboards. The marginal difference
     * between B and C and between E and F is half of that between other
     * letters. 
     */
    static char cum_letter_values[] = {
    //  A  B  C  D  E  F  G     LETTERS
    //  2, 2, 1, 2, 2, 1, 2     MARGINAL VALUES
        0, 2, 3, 5, 7, 8, 10 // CUMULATIVE VALUES
    };
    
    int key = 
        cum_letter_values[note->letter - 'A'] -
        cum_letter_values[BASE_LETTER  - 'A'] +
        12 * (note->octave - BASE_OCTAVE) +
        note->accidental;
    
    return 1 << key;
}

// Returns which keys should be pressed now
keys_t get_pressable_keys() {
    keys_t pressable_keys = 0;
    
    const struct quanta *quanta = &song->quantas[quanta_no];
    const struct note *chord_notes = quanta->chord_notes;
    const struct note_duration_pair *melody_notes = quanta->melody_notes;
    
    // Look at all notes in the current chord
    if (chord_notes) {
        int i;
        for (i = 0; i < quanta->num_chord_notes; i++) {
            pressable_keys |= note_to_key(&chord_notes[i]);
        }
    }
    
    // Look at all notes in the current melody
    if (melody_notes) {
        int i;
        for (i = 0; i < quanta->num_melody_notes; i++) {
            pressable_keys |= note_to_key(&melody_notes[i].note);
        }
    }
    
    return pressable_keys;
}

void player_start_song(const struct song *new_song) {
    // Set up song state
    song = new_song;
    quanta_no = 0;
    measure_start_quanta_no = 0;
    
    // Clear the screen and set up the song title
    tft_fillScreen(ILI9340_WHITE);
    const int text_size = 2;
    short x_offset = (tft_width() - text_size * 6 * strlen(song->title)) / 2;
    tft_setTextSize(text_size);
    tft_setTextColor(ILI9340_BLACK);
    tft_setCursor(x_offset, 20);
    tft_writeString(song->title);
    
    leds_set(get_pressable_keys()); // Set up LEDs to starting pressable values
    display_sheet_music();          // Set up initial sheet music display
}

int player_update() {
    keys_t pressable_keys = get_pressable_keys();
    keys_t pressed_keys = keyboard_poll();
    
    // If keys which are pressed is a superset of keys which need to pressed,
    // then we should advance to the next quanta
    bool all_pressed = pressable_keys == (pressable_keys & pressed_keys);
    
    if (all_pressed) {
        keyboard_wait_for_release();    // Wait for current keys to be released
        if (++quanta_no >= song->num_quantas) {
            // Turn off LEDs and return 1 if song is over
            leds_set(0);
            return 1;
        }
        leds_set(get_pressable_keys()); // Set LEDs to new pressable keys
        if (song->quantas[quanta_no].time >=
            song->quantas[measure_start_quanta_no].time + 16) {
            // Update current measure if necessary
            measure_start_quanta_no = quanta_no;
        }
        display_sheet_music(); // Update displayed sheet music
    }
        
    return 0;    
}
