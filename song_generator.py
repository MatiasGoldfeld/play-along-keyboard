import sys
import os


cur_id = 0


def gen_unique_name():
    # Generates a unique name for C variables
    global cur_id
    cur_id += 1
    return f"id{cur_id}"


class Note:
    def __init__(self, note):
        self.letter = note[0]
        if len(note) == 2:
            self.accidental = 0
            self.octave = int(note[1])
        elif len(note) == 3 and note[1] in ('#', 'b'):
            if note[1] == '#':
                self.accidental = 1
            elif note[1] == 'b':
                self.accidental = -1
            else:
                print("Error, unknown accidental: ", note)
            self.octave = int(note[2])
        else:
            print("Error, note of irregular length detected: ", note)

    def to_c(self):
        return f"{{'{self.letter}', {self.accidental}, {self.octave}}}"


class NoteDurationPair:
    def __init__(self, note, duration):
        self.note = Note(note)
        self.duration = int(duration)

    def to_c(self):
        return f"{{{self.note.to_c()}, {self.duration}}}"


class Quanta:
    def __init__(self, time, chord_identifier, chord_duration, chord_notes, melody_note_duration_pairs):
        self.time = int(time, 16)

        self.chord_identifier = chord_identifier
        self.chord_duration = int(chord_duration)
        self.chord_notes = chord_notes

        self.melody_note_duration_pairs = melody_note_duration_pairs

    def to_c(self):
        reqs = ""

        chord_notes_name = "NULL";
        if len(self.chord_notes) > 0:
            chord_notes = ','.join([chord_note.to_c() for chord_note in self.chord_notes])
            chord_notes_name = gen_unique_name()
            reqs += f"const struct note {chord_notes_name}[] = {{{chord_notes}}};\n"

        melody_note_duration_pairs_name = "NULL";
        if len(self.melody_note_duration_pairs) > 0:
            melody_notes = ','.join([melody_note.to_c() for melody_note in self.melody_note_duration_pairs])
            melody_note_duration_pairs_name = gen_unique_name()
            reqs += f"const struct note_duration_pair {melody_note_duration_pairs_name}[] = {{{melody_notes}}};\n"

        quanta_c = f"""{{
    {self.time},
    {'"' + self.chord_identifier + '"' if len(self.chord_notes) > 0 else "NULL"},
    {self.chord_duration},
    {len(self.chord_notes)},
    {chord_notes_name},
    {len(self.melody_note_duration_pairs)},
    {melody_note_duration_pairs_name}
}}
"""

        return reqs, quanta_c


class Song:
    def __init__(self, song_file):
        lines = song_file.splitlines()
        self.title = lines[0]
        self.millis_per_beat = round(60000 / int(lines[1]))
        self.base_sheet_octave = int(lines[2])
        self.quantas = []
        for line in lines[3:]:
            parts = [part.strip() for part in line.split(',')]

            time = parts[0]

            chord_identifier = ""
            chord_duration = 0
            chord_notes = []
            if len(parts) > 1 and parts[1] != "":
                chords = parts[1].split(' ')
                chord_identifier = chords[0]
                chord_duration = chords[1]
                chord_notes = [Note(note) for note in chords[2:]]

            melody_pairs = []
            if len(parts) > 2 and parts[2] != "":
                melody = parts[2].split(' ')
                if len(melody) % 2 != 0:
                    print("Error, melody not in pairs: ", parts[2])
                melody_pairs = [NoteDurationPair(melody[i], melody[i + 1]) for i in range(0, len(melody), 2)]

            self.quantas.append(Quanta(time, chord_identifier, chord_duration, chord_notes, melody_pairs))

    def to_c(self):
        reqs = ""
        quantas_c = []

        for quanta in self.quantas:
            quanta_reqs, quanta_c = quanta.to_c()
            reqs += quanta_reqs
            quantas_c.append(quanta_c)

        quantas_name = gen_unique_name()
        reqs += f"""const struct quanta {quantas_name}[] = {{
{','.join(quantas_c)}
}};
"""

        song_c = f"""{{
    "{self.title}",
    {self.millis_per_beat},
    {self.base_sheet_octave},
    {len(self.quantas)},
    {quantas_name}
}}
"""

        return reqs, song_c


def create_c_file(location, song_dir):
    reqs = ""
    songs_c_list = []

    for song_file in os.listdir(song_dir):
        song_reqs, song_c = Song(open(os.path.join(song_dir, song_file), 'r', ).read()).to_c()
        reqs += song_reqs
        songs_c_list.append(song_c)

    songs_c = ',\n'.join(songs_c_list)

    songs_file_contents = f"""#include "song.h"
#include "stddef.h"

/* THIS IS A FILE AUTOMATICALLY GENERATED BY song_generator.py */

{reqs}

const struct song all_songs[] = {{
    {songs_c},
    {{"Freeplay Mode", 0, 0, 0, 0}}
}};

const unsigned int n_songs = {len(songs_c_list) + 1};
const struct song * const songs = all_songs;
"""

    open(location, 'w').write(songs_file_contents)


if __name__ == "__main__":
    create_c_file("song.c", sys.argv[1])
