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

#include "ExpCreator.h"
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
	options.process(argc, argv);

	ExpCreator creator;

	if (options.getString("input") == "-") {
		cerr << "Error: cannot read from standard input yet." << endl;
		exit(1);
	} else {
		creator.readMidiFile(options.getString("input"));
	}

	creator.addExpression();

	if (options.getString("output") == "-") {
		cerr << "Error: cannot write to standard output yet." << endl;
		exit(1);
	} else {
		creator.writeMidiFile(options.getString("output"));
	}

	if (options.getBoolean("print-expression")) {
		creator.printExpression(cout);
	}

	return 0;
}


