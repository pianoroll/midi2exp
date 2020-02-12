//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp
// Creation Date: Tue Jun 19 16:15:12 PDT 2018
// Last Modified: Fri Mar 22 17:15:42 PDT 2018
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
	options.define("a|adjust-hole-lengths=b", "adjust hole lengths to simulate tracker bar width");
	options.define("b|print-boolean-states=b", "print boolean states with expressions");
	options.define("d|punch-diameter=d:21.5", "hole punch diameter in pixels at 300 dpi");
	options.define("e|print-expression=b", "print expression time values");
	options.define("f|punch-fraction=d:0.75", "fraction of hole diameter to extend notes by to emulate tracker bar width");
	options.define("k|trackerbar-diameter=d:16.7", "tracker bar height in pixels at 300 dpi");
	options.define("r|remove-expression-tracks=b", "remove expression tracks");
	options.define("t|tempo=d:100", "tempo adjustment for pianoroll file");
	options.define("x|display-extended-expression-info=b", "display booleans needed to create expression");
	options.define("w|red|red-welte=b:", "set tempo for red welte rolls (95)");
	options.define("g|green|green-welte=b:", "process a green welte roll");

	options.define("sd|slow-decay-rate=d:2380", "Slow decay rate (Red Welte)"); // 2380
	options.define("fc|fast-crescendo=d:300", "Fast crescendo (Red Welte)");    //180 --> 300
	options.define("fd|fast-decrescendo=d:300", "Fast descrescendo (Red Welte)"); //17 ---> 300
	options.define("wp|welte-piano=d:23.0", "(Red Welte)");
	options.define("wmf|welte-mezzo-forte=d:60.0", "(Red Welte)");
	options.define("wf|welte-forte=d:89.0", "(Red Welte)");
	options.define("wl|welte-loud=d:70.0", "(Red Welte)");
	options.define("v|version=s", "Add version number metadata");
	options.define("i|ai|acceleration-inches=d:12.0", "acceleration update for every xx inches");
	options.define("p|ap|acceleration-percent=d:0.22", "acceleration percent (based on every x inches)");

	options.process(argc, argv);

	Expressionizer creator;
	creator.setupRedWelte();
	if (options.getBoolean("green")) {
		creator.setupGreenWelte();
	}

	creator.setPunchDiameter(options.getDouble("punch-diameter"));
	creator.setTrackerbarDiameter(options.getDouble("trackerbar-diameter"));
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

	// green welte default tempo is 72.2222 if not specified.
	if (options.getBoolean("red-welte")) {
		creator.setRollTempo(95);    //94.6
	} else if (options.getBoolean("tempo")) {
		creator.setRollTempo(options.getDouble("tempo"));
	}

	if (options.getBoolean("welte-piano")) {
		creator.setWelteP(options.getDouble("welte-piano"));
	}
	if (options.getBoolean("welte-mezzo-forte")) {
		creator.setWelteMF(options.getDouble("welte-mezzo-forte"));
	}
	if (options.getBoolean("welte-forte")) {
		creator.setWelteF(options.getDouble("welte-forte"));
	}
	if (options.getBoolean("welte-loud")) {
		creator.setWelteLoud(options.getDouble("welte-loud"));
	}

	if (options.getBoolean("slow-decay-rate")) {
		creator.setSlowDecayRate(options.getDouble("slow-decay-rate"));
	}
	if (options.getBoolean("fast-crescendo")) {
		creator.setFastCrescendo(options.getDouble("fast-crescendo"));
	}
	if (options.getBoolean("fast-decrescendo")) {
		creator.setFastDecrescendo(options.getDouble("fast-decrescendo"));
	}

	if (options.getArgCount() < 2) {
		cerr << "Error: cannot write to standard input yet." << endl;
		exit(1);
	}
	if (options.getBoolean("adjust-hole-lengths")) {
		creator.applyTrackBarWidthCorrection();
	}

	if (options.getBoolean("version")) {
		creator.setVersion(options.getString("version"));
	}

	double inches = options.getDouble("acceleration-inches");
	double percent = options.getDouble("acceleration-percent");
	creator.setAcceleration(inches, percent);

	creator.addExpression();
	creator.setPianoTimbre();
	creator.writeMidiFile(options.getArg(2));
	//creator.printVelocity();   // for debug

	if (options.getBoolean("print-expression")) {
		creator.printExpression(cout, options.getBoolean("display-extended-expression-info"));
	}

	return 0;
}


