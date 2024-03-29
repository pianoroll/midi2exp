//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp
// Creation Date: Tue Jun 19 16:15:12 PDT 2018
// Last Modified: Wed Aug 4  15:15:42 PDT 2021
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
	options.define("l|licensee|licensee-welte=b:", "process a welte licensee roll");
	options.define("h|88-note=b:", "process an 88-note roll");
	options.define("u|duo-art=b:", "process a duo-art roll");

	//red
	// options.define("sd|slow-decay-rate=d:2380", "Slow decay rate (Red Welte)"); // 2380
	// options.define("fc|fast-crescendo=d:300", "Fast crescendo (Red Welte)");    //180 --> 300
	// options.define("fd|fast-decrescendo=d:300", "Fast descrescendo (Red Welte)"); //17 ---> 300
	// options.define("wp|welte-piano=d:23.0", "(Red Welte)");
	// options.define("wmf|welte-mezzo-forte=d:60.0", "(Red Welte)");
	// options.define("wf|welte-forte=d:89.0", "(Red Welte)");
	// options.define("wl|welte-loud=d:70.0", "(Red Welte)");
	// options.define("ac|accel-ft-per-min2=d:0.3147", "acceleration in feet per minute^2");

	// green
	options.define("sd|slow-decay-rate=d:2366", "Slow decay rate (Red Welte)"); // 2380
	options.define("fc|fast-crescendo=d:254", "Fast crescendo (Red Welte)");    //180 --> 300
	options.define("fd|fast-decrescendo=d:269", "Fast descrescendo (Red Welte)"); //17 ---> 300
	options.define("wp|welte-piano=d:35.0", "Minimum velocity");
	options.define("wmf|welte-mezzo-forte=d:60.0", "MezzoForte velocity)");
	options.define("wf|welte-forte=d:90.0", "Forte velocity");
	options.define("wl|welte-loud=d:70.0", "Loud velocity");

	options.define("v|version=s", "Add version number metadata");
	options.define("ac|accel-ft-per-min2=d:0.2", "acceleration in feet per minute^2");

	options.process(argc, argv);

	Expressionizer creator;
	creator.setupRedWelte();
	if (options.getBoolean("green")) {
		cout << "Processing Green Welte rolls" << endl;
		creator.setupGreenWelte();
	}
	else if (options.getBoolean("licensee")) {
		cout << "Processing Welte Licensee rolls" << endl;
		creator.setupLicenseeWelte();
	}
	else if (options.getBoolean("88-note")) {
		cout << "Processing 88-note rolls" << endl;
		creator.setup88Roll();
	}
	else if (options.getBoolean("duo-art")) {
		cout << "Processing Duo-art rolls" << endl;
		creator.setupDuoArt();
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

	// default acceleration is 0.2, except for red Welte rolls
	// green welte default tempo is 72.2222 if not specified.
	if (options.getBoolean("red-welte")) {
		creator.setRollTempo(94.6);    //94.6
		cout << "setting red welte tempo 94.6" << endl;
		creator.setAcceleration(0.3147);
	}
	else if (options.getBoolean("green-welte")){
		creator.setRollTempo(72.2);
		cout << "setting green welte tempo 72.2" << endl;
	}
	// welte licensee tempo to be 79.8 by examining the test roll
	else if (options.getBoolean("licensee-welte")){
		creator.setRollTempo(79.8);
		cout << "setting welte licensee tempo 79.8" << endl;
	}
	else if (options.getBoolean("88-note")){
		creator.setRollTempo(60);
		cout << "setting 88-note roll tempo 60" << endl;
	}
	else if (options.getBoolean("duo-art")){
		creator.setRollTempo(70);
		cout << "setting duo-art roll tempo 70" << endl;
	}
	else if (options.getBoolean("tempo")) {
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

	if (options.getBoolean("accel-ft-per-min2")) {
		creator.setAcceleration(options.getDouble("accel-ft-per-min2"));
	}

	creator.addExpression();
	creator.setPianoTimbre();
	creator.writeMidiFile(options.getArg(2));
	//creator.printVelocity();   // for debug

	if (options.getBoolean("print-expression")) {
		creator.printExpression(cout, options.getBoolean("display-extended-expression-info"));
	}

	return 0;
}


