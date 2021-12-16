#ifndef _PLAYER_H
#define _PLAYER_H

/* PLAYER.H
 * Manages song playing songs with input from
 * the keyboard and output to the LEDs.
 */

// Sets up state to start playing [new_song].
void player_start_song(const struct song *new_song);

// Should be called frequently when playinga song.
// Returns 1 when the song is finished and 0 otherwise.
int  player_update();

#endif
