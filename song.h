#ifndef _SONG_H
#define _SONG_H

/* SONG.H
 * Defines how songs are represented in memory, and contains loaded songs.
 */

typedef unsigned int beat_duration;

struct note {
    char letter;            // 'A' - 'G'
    char accidental;        // -1 (flat), 0 (natural), or 1 (sharp)
    unsigned char octave;
};

struct note_duration_pair {
    struct note note;
    beat_duration duration; 
};

struct quanta {
    beat_duration time;
    
    char * const chord;
    beat_duration chord_duration;
    unsigned int num_chord_notes;
    const struct note * const chord_notes;                  // No chord if NULL
    
    unsigned int num_melody_notes;
    const struct note_duration_pair * const melody_notes;   // No melody if NULL
};

struct song {
    char * const title;
    unsigned int millis_per_beat;
    unsigned char base_sheet_octave;
    unsigned int num_quantas;
    const struct quanta * const quantas;
};

// Number of songs in [songs].
const unsigned int n_songs;

// Array of all songs loaded into memory.
const struct song * const songs;

#endif
