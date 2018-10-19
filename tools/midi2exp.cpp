//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp
// Creation Date: Tue Jun 19 16:15:12 PDT 2018
// Last Modified: Thu Oct  4 17:15:42 PDT 2018
// Filename:      midi2exp/tools/midi2exp.cpp
// Website:       https://github.com/pianoroll/midi2exp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// description:   Command-line interface to interpret expression for piano roll MIDI files.
//

#include "Expressionizer.h"
#include "Options.h"

#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace smf;

int main(int argc, char** argv) {
	Options options;
	options.define("i|input=s:-",  "Input MIDI file name");
	options.define("o|output=s:-", "Output MIDI file name");
	options.define("e|print-expression=b", "print expression time values");
	options.define("r|remove-expression-tracks=b", "remove expression tracks");
	options.define("b|print-boolean-states=b", "print boolean states with expressions");
	options.define("d|punch-diameter=d:21.5", "hole punch diameter in pixels at 300 dpi");
	options.define("f|punch-fraction=d:0.25", "fraction of hole diameter to extend notes by to emulate tracker bar width");
	options.define("a|adjust-hole-lengths=b", "adjust hole lengths to simulate tracker bar width");
	options.process(argc, argv);

	Expressionizer creator;

	creator.setPunchDiameter(options.getDouble("punch-diameter"));
	creator.setPunchExtensionFraction(options.getDouble("punch-fraction"));
	if (options.getBoolean("remove-expression-tracks")) {
		creator.removeExpressionTracksOnWrite();
	}

	if (options.getArgCount() == 0) {
		cerr << "Error: cannot read from standard input yet." << endl;
		exit(1);
	}
	if (options.getArgCount() >= 1) {
		creator.readMidiFile(options.getArg(1));
	}
	if (options.getArgCount() < 2) {
		cerr << "Error: cannot write to standard input yet." << endl;
		exit(1);
	}
	if (options.getBoolean("adjust-hole-lengths")) {
		creator.applyTrackBarWidthCorrection();
	}

	creator.addExpression();
	creator.writeMidiFile(options.getArg(2));

	if (options.getBoolean("print-expression")) {
		creator.printExpression(cout);
	}

	return 0;
}


