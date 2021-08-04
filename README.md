# midi2exp
This repository converts pianoroll raw MIDI files into expressionized MIDI files

## Compiling

To compile the library and tools, type:

```bash
make
```

GNU make must be installed, and gcc version 4.9 or higher (or most versions of clang on macOS).

## Run
```bash
./bin/midi2exp/ -ar <raw_midi_file.mid> <exp_midi_file.mid>
```

## Options
-a: adjust hole lengths to simulate tracker bar width \
-r: red welte rolls \
-g: green welte rolls \
-l: welte licensee rolls \
-h: 88-note rolls \
-r: remove expression tracks