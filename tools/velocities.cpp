//
// Programmer:    Craig Stuart Sapp
// Creation Date: Fri Oct  5 10:16:17 PDT 2018
// Last Modified: Fri Oct  5 10:16:19 PDT 2018
// Filename:      midi2exp/tools/velocities.cpp
// Website:       https://github.com/pianoroll/midi2exp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// description:   Extract a list of velocities for a particular register.
//
// See https://stackoverflow.com/questions/13869439/gnuplot-how-to-increase-the-width-of-my-graph
// To set a fixed x-axis scaling.
// 
//

#include "MidiFile.h"
#include "Options.h"

#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;
using namespace smf;

string filename;
Options options;

ostream& printGnuplotHeader(ostream& out = std::cout);

int main(int argc, char** argv) {
	options.define("tick|ticks=b",    "print time as tick values");
	options.define("s|sec|seconds=b", "print time in units of seconds (default)");
	options.define("m|ms|millisec|millisecs|millisecond|milliseconds=b", "print time in units of milliseconds");
	options.define("b|bass=b",        "print times for bass register (default)");
	options.define("t|treble=b",      "print times for treble register");
	options.define("g|gnuplot=b",     "print gnuplot script header");
	options.process(argc, argv);

	MidiFile infile;

	if (options.getArgCount() == 0) {
		infile.read(cin);
	} else {
		filename = options.getArg(1);
		infile.read(filename);
	}

	if (infile.getTrackCount() < 3) {
		cerr << "Error: not enough tracks in MIDI file." << endl;
		exit(1);
	}

	int bass_track = 1;
	int treble_track = 2;

	int track = bass_track;
	if (options.getBoolean("treble")) {
		track = treble_track;
	}

	bool secondsQ = options.getBoolean("seconds");
	bool milliQ   = options.getBoolean("milliseconds");
	bool ticksQ   = options.getBoolean("ticks");
	if (!ticksQ) {
		infile.doTimeAnalysis();
	}

	if (options.getBoolean("gnuplot")) {
		printGnuplotHeader();
	}

	double lasttick = -1;
	double tick     = -1;
	double time;
	for (int i=0; i<infile[track].getEventCount(); i++) {
		MidiEvent* me = &infile[track][i];
		if (!me->isNoteOn()) {
			continue;
		}
		lasttick = tick;
		tick = me->tick;
		if (lasttick == tick) {
			// ignoring simultaneties for now
			continue;
		}
		if (ticksQ) {
			cout << tick << "\t" << me->getP2() << "\n";
		} else if (milliQ) {
			cout << int(me->seconds * 1000.0 + 0.5) << "\t" << me->getP2() << "\n";
		} else { // display time in seconds by default
			cout << me->seconds << "\t" << me->getP2() << "\n";
		}
	}

	return 0;
}


//////////////////////////////
//
// printGnuplotHeader -- Prefix a script to plot the data with gnuplot.
//

ostream& printGnuplotHeader(ostream& out) {

	// generate an output filename for the generated SVG image:
	string basename;
	if (!filename.empty()) {
		auto path = filename.rfind("/");
		if (path != string::npos) {
			basename = filename.substr(path);
		} else {
			basename = filename;
		}
		auto extension = basename.rfind(".");
		if ((extension != string::npos) && (extension != 0)) {
			basename = basename.substr(0, extension);
		}
	}
	if (basename.empty()) {
		basename = "output";
	}

	out << "#!/usr/local/bin/gnuplot" << endl;
	out << endl;
	if (options.getBoolean("ticks")) {
		out << "set xlabel \"time [ticks]\"" << endl;
	} else if (options.getBoolean("milliseconds")) {
		out << "set xlabel \"time [milliseconds]\"" << endl;
	} else {
		out << "set xlabel \"time [seconds]\"" << endl;
	}
	out << "set ylabel \"velocity\"" << endl;
	out << "set terminal svg size 1200,300" << endl;
	out << "set output \"" << basename << ".svg\"" << endl;
	out << "plot \"-\" with lines linecolor rgb \"purple\"" << endl;
	out << endl;

	return out;
}



