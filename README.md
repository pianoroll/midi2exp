# midi2exp
This repository converts pianoroll raw MIDI files into expressionized MIDI files

## Compiling

To compile the library and tools, type:

```bash
make
```

GNU make must be installed, and gcc version 4.9 or higher (or most versions of clang on macOS).

## Input
The input MIDI file must have 4 tracks, with track 1 and 2 for treble and bass notes, and track 3 and 4 for treble and bass expressions.

## Run
```bash
./bin/midi2exp/ -awr <raw_midi_file.mid> <exp_midi_file.mid>
```

## Options
-a: adjust hole lengths to simulate tracker bar width \
-w: process red welte rolls \
-g: process green welte rolls \
-l: process welte licensee rolls \
-h: 88-note rolls \
-r: remove expression tracks