//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp (translation to C++)
// Creation Date: Thu Oct  4 16:32:27 PDT 2018
// Last Modified: Fri Mar 22 16:25:41 PDT 2018
// Filename:      midi2exp/src/Expressionizer.cpp
// Website:       https://github.com/pianoroll/midi2exp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// description:   Class that applies expression note velocities.
//

#include "Expressionizer.h"

#include <algorithm>
#include <iostream>

using namespace std;
using namespace smf;


//////////////////////////////
//
// Expressionizer::Expressionizer -- Constructor.
//

Expressionizer::Expressionizer(void) {
  	slow_step   =  cresc_rate * welte_mf            / slow_decay_rate;
  	fastC_step  =  cresc_rate * (welte_f - welte_p) / fastC_decay_rate;
  	fastD_step  = -cresc_rate * (welte_f - welte_p) / fastD_decay_rate;
	// cerr << "CRESC_RATE " << cresc_rate << endl;
	// cerr << "SLOW STEP " << slow_step << endl;
	// cerr << "FASTC STEP " << fastC_step << endl;
	// cerr << "FASTD STEP " << fastD_step << endl;
}



//////////////////////////////
//
// Expressionizer::Expressionizer -- Deconstructor.
//

Expressionizer::~Expressionizer(void) {
	// do nothing
}



//////////////////////////////
//
// Expressionizer::addExpression -- Add expression velocities to a MIDI file.
//

void Expressionizer::addExpression(void) {
	setPan();

	calculateRedWelteExpression("left_hand");
	calculateRedWelteExpression("right_hand");

	applyExpression("left_hand");
	applyExpression("right_hand");

	addSustainPedalling(midi_data, TREBLE_EXPRESSION);
	addSoftPedalling(midi_data, BASS_EXPRESSION);
}



//////////////////////////////
//
// Expressionizer::addSustainPedalling --
//

void Expressionizer::addSustainPedalling(MidiFile& midifile, int sourcetrack) {
	int count = midifile.getEventCount(sourcetrack);
	MidiEvent me_bass;
	MidiEvent me_treble;
	me_bass.track = 1;
	me_treble.track = 2;
	for (int i=0; i<count; i++) {
		if (!midifile[sourcetrack][i].isNoteOn()) {
			continue;
		}
		int key = midifile[sourcetrack][i].getKeyNumber();
		me_bass.tick = midifile[sourcetrack][i].tick;
		me_treble.tick = midifile[sourcetrack][i].tick;
		if (key == PedalOnKey) {
			me_bass.makeController(1, 64, 127);
			midifile.addEvent(me_bass);
			me_treble.makeController(2, 64, 127);
			midifile.addEvent(me_treble);
		} else if (key == PedalOffKey) {
			me_bass.makeController(1, 64, 0);
			midifile.addEvent(me_bass);
			me_treble.makeController(2, 64, 0);
			midifile.addEvent(me_treble);
		}
	}
	midifile.sortTracks();

}



//////////////////////////////
//
// Expressionizer::addSoftPedalling --
//

void Expressionizer::addSoftPedalling(MidiFile& midifile, int sourcetrack) {
	int count = midifile.getEventCount(sourcetrack);
	MidiEvent me_bass;
	MidiEvent me_treble;
	me_bass.track = 1;
	me_treble.track = 2;
	for (int i=0; i<count; i++) {
		if (!midifile[sourcetrack][i].isNoteOn()) {
			continue;
		}
		int key = midifile[sourcetrack][i].getKeyNumber();
		me_bass.tick = midifile[sourcetrack][i].tick;
		if (key == SoftOnKey) {
			me_bass.makeController(1, 67, 127);
			midifile.addEvent(me_bass);
			me_treble.makeController(2, 67, 127);
			midifile.addEvent(me_treble);
		} else if (key == SoftOffKey) {
			me_bass.makeController(1, 67, 0);
			midifile.addEvent(me_bass);
			me_treble.makeController(2, 67, 0);
			midifile.addEvent(me_treble);
		}
	}
	midifile.sortTracks();
}



//////////////////////////////
//
// Expressionizer::setRollTempo -- Set the tempo of the roll
// input: tempo (98.4252 for red welte), (default 300 dpi, 6 as multiplier)
//
void Expressionizer::setRollTempo(double tempo) {
	midi_data.setTPQ(int(tempo * 6 + 0.5));
	updateMidiTimingInfo();
}

//////////////////////////////
//
// Expressionizer::setPan -- Set a panning position for the bass and treble
//    halfs of the piano.  Note: the original midi file may already have
//    panning information (32 left 96 right). The following function should
//    update that message with a new value, and otherwise add a new panning
//    message.
//

void Expressionizer::setPan(void) {
	// pan_cont_num: 10 == pan controller
	int pan_cont_num = 10;

	MidiEvent* bass_event = NULL;
	MidiEvent* treble_event = NULL;
	MidiEventList& bass_note_list   = midi_data[bass_ch];
	MidiEventList& treble_note_list = midi_data[treble_ch];

	// search for an existing pan message in each note list, and use
	// that value if found; otherwise a new pan message will be added
	//	to the MIDI file.

	for (int i=0; i<bass_note_list.getEventCount(); i++) {
		if (!bass_note_list[i].isController()) {
			continue;
		}
		if (bass_note_list[i].getP1() == pan_cont_num) {
			bass_event = &bass_note_list[i];
		}
	}

	for (int i=0; i<treble_note_list.getEventCount(); i++) {
		if (!treble_note_list[i].isController()) {
			continue;
		}
		if (treble_note_list[i].getP1() == pan_cont_num) {
			treble_event = &treble_note_list[i];
		}
	}

	int tick = 0;

	if (bass_event) {
		bass_event->setP2(pan_bass);
	} else {
		midi_data.addController(bass_track, tick, bass_ch, pan_cont_num, pan_bass);
	}

	if (treble_event) {
		treble_event->setP2(pan_treble);
	} else {
		midi_data.addController(treble_track, tick, treble_ch, pan_cont_num, pan_treble);
	}

}



//////////////////////////////
//
// Expressionizer::readMidiFile --
//

bool Expressionizer::readMidiFile(std::string filename) {
	bool status =  midi_data.read(filename);
	if (!status) {
		return status;
	}
	updateMidiTimingInfo();
	return true;
}



//////////////////////////////
//
// Expressionizer::updateMidiTimingInfo --
//

void Expressionizer::updateMidiTimingInfo(void) {
	midi_data.doTimeAnalysis();
	midi_data.linkNotePairs();
}



//////////////////////////////
//
// Expressionizer::writeMidiFile --
//

bool Expressionizer::writeMidiFile(std::string filename) {
	if ((midi_data.getTrackCount() == 5) && delete_expresison_tracks) {
		midi_data.deleteTrack(4);
		midi_data.deleteTrack(3);
	}

	return midi_data.write(filename);
}




/*  Probably already in input MIDI file:

	def addPedal(self):
		// adding soft pedal
		softOn = 0.0
		softOff = 0.0

		// add soft pedal
		for note in self.bass_exp.notes:
			if note.pitch == 21:
				if softOff != 0.0:
					print 'Warning! previous soft pedal not off before ' + str(note.start)
				softOff = 0.0
				softOn = note.start
			elif note.pitch == 20:
				//if softOn == 0.0:
				//    print "Warning no soft pedal on detected before " + str(note.start)
				softOff = note.end
				//print 'soft pedal on from: ' + str(softOn) + ' to: ' + str(softOff)
				softControlOn = pretty_midi.ControlChange(67,70,softOn)
				softControlOff = pretty_midi.ControlChange(67,0,softOff)
				self.left.control_changes.append(softControlOn)
				self.left.control_changes.append(softControlOff)
				self.right.control_changes.append(softControlOn)
				self.right.control_changes.append(softControlOff)

				softOn = 0.0
				softOff = 0.0


		// add sustainPedal
		pedalOn = 0.0
		pedalOff = 0.0

		for note in self.treble_exp.notes:
			if note.pitch == 106:
				//if pedalOff != 0.0:
				//    print 'Warning! previous sustain pedal not off before ' + str(note.start)
				pedalOff = 0.0
				pedalOn = note.start
			elif note.pitch == 107:
				//if pedalOn == 0.0:
				//    print "Warning no sustain pedal on detected before " + str(note.start)
				pedalOff = note.end
				sustainControlOn = pretty_midi.ControlChange(64,70,pedalOn)
				sustainControlOff = pretty_midi.ControlChange(64,0,pedalOff)
				self.left.control_changes.append(sustainControlOn)
				self.left.control_changes.append(sustainControlOff)
				self.right.control_changes.append(sustainControlOn)
				self.right.control_changes.append(sustainControlOff)

				pedalOn = 0.0
				pedalOff = 0.0

*/



//////////////////////////////
//
// Expressionizer::applyExpression --
//
// Input variable "option" can take two values: "left_hand" or "right_hand".
//

void Expressionizer::applyExpression(std::string option) {
	int track;
	vector<double>* timeline;

	if (option == "left_hand") {
		track = bass_track;
		timeline = &exp_bass;
	} else {
		track = treble_track;
		timeline = &exp_treble;
	}
	MidiEventList& mynotes = midi_data[track];

	for (int i=0; i<mynotes.getEventCount(); i++) {
		MidiEvent* me = &mynotes[i];
		if (!me->isNoteOn()) {
			continue;
		}

		// find closest velocity of the begin
		int starttime = int(me->seconds * 1000.0 + 0.5);
		int ms = std::min(starttime, (int)timeline->size() - 1);
		int velocity = int(timeline->at(ms) + 0.5);

		if (velocity == 0) {
			velocity = getPreviousNonzero(*timeline, ms);
		}

		if (option == "left_hand") {
			velocity = std::max(velocity + left_adjust, 0);
		}

		// if still equals 0, map it to 60
		if (velocity == 0) {
			velocity = 60;
		}

		me->setVelocity(velocity);

	}

}



//////////////////////////////
//
// Expressionizer::getPreviousNonzero -- Get the previous nonzero value
//     from a vector.
//

double Expressionizer::getPreviousNonzero(vector<double>& myArray,
		int start_index) {
	for (int i=start_index; i>=0; i--) {
		if (myArray[i] > 0.0) {
			return myArray[i];
		}
	}
	// Could not find a previous value; return welte_mf:
	return welte_mf;
}



//////////////////////////////
//
// Expressionizer::calculateRedWelteExpression --
//
//		As of 2018-04-06, some modification (F: fast crescendo -- length
//    of perforation) (F+slow crescendo: fastest crescendo)
//		Using Peter's velocity mapping: min30 MF60 Loud70 Max85
//		dynamic range default 1.2, making welte_mf: 80, Max (F): 96, and Min (P) 66
//		left_hand: 12 less than right hand
//
//		According to the following expression code of Red Welte
//		Midi track 3 (zero offset):
//		   14: (1)Bass MF off
//		   15: (2)Bass MF on
//		   16: (3)Bass Crescendo off (Slow Crescendo)
//		   17: (4)Bass Crescendo on
//		   18: (5)Bass Forzando off  (Fast Crescendo)
//		   19: (6)Bass Forzando on
//		   20: (7)Soft-pedal off
//		   21: (8)Soft-pedal on
//		   22: Motor off
//		   23: Motor on
//
//		Midi track 4 (zero offset):
//		   104: Rewind
//		   105: Electric cutoff
//		   106: (8)Sustain pedal on
//		   107: (7)Sustain pedal off
//		   108: (6)Treble Forzando on   (Fast Crescendo)
//		   109: (5)Treble Forzando off
//		   110: (4)Treble Crescendo on  (Slow Crescendo)
//		   111: (3)Treble Crescendo off
//		   112: (2)Treble MF on
//		   113: (1)Treble MF off
//
//    pianoside = "left_hand" or "right_hand";
//

void Expressionizer::calculateRedWelteExpression(std::string option) {
	int track_index;
	vector<double>* expression_list;
	vector<double>* isSlowC;
	vector<double>* isFastC;
	vector<double>* isFastD;

	// Initial Setup of the expression curve
	if (option == "left_hand") {
		track_index = bass_exp_track;
		expression_list = &exp_bass;
		isSlowC = &isSlowC_bass;
		isFastC = &isFastC_bass;
		isFastD = &isFastD_bass;
	} else {
		track_index = treble_exp_track;
		expression_list = &exp_treble;
		isSlowC = &isSlowC_treble;
		isFastC = &isFastC_treble;
		isFastD = &isFastD_treble;
	}

	// exp_notes = notes for the expessions being processed.
	MidiEventList& exp_notes = midi_data[track_index];

	// length of the MIDI file in milliseconds (plus an extra millisecond
	// to avoid problems):
	int exp_length = midi_data.getFileDurationInSeconds() * 1000 + 1;


	expression_list->resize(exp_length);

	// set all of the times to piano by default:
	std::fill(expression_list->begin(), expression_list->end(), welte_p);

	vector<bool> isMF(exp_length, false);    // setting up the upper/lower bound

	isSlowC->resize(exp_length);
	isFastC->resize(exp_length);
	isFastD->resize(exp_length);
	std::fill(isSlowC->begin(), isSlowC->end(), false);  // is slow crescendo on?
	std::fill(isFastC->begin(), isFastC->end(), false);  // is fast crescendo on?
	std::fill(isFastD->begin(), isFastD->end(), false);  // is fast decrescendo on?

	// Lock and Cancel
	bool valve_mf_on    = false;
	bool valve_slowc_on = false;

	int valve_mf_starttime    = 0;        // 0 for off
	int valve_slowc_starttime = 0;

	// First pass: For each time section calculate the current boolean
	// state of each expression.

	for (int i=0; i<exp_notes.getEventCount(); i++) {
		MidiEvent* me = &exp_notes[i];
		if (!me->isNoteOn()) {
			continue;
		}
		int exp_no = me->getKeyNumber();  // expression number
		int st = int(me->seconds * 1000.0 + 0.5);  // start time in milliseconds
		int et = int((me->seconds + me->getDurationInSeconds()) * 1000.0 + 0.5);

		if ((exp_no == 14) || (exp_no == 113)) {
			// MF off
			if (valve_mf_on) {
				for (int j=valve_mf_starttime; j<st; j++) {
					// record MF Valve information for previous
					isMF[j] = true;
				}
			}
			valve_mf_on = false;

		} else if ((exp_no == 15) || (exp_no == 112)) {
			// MF on, just update the start Time
			if (!valve_mf_on) {    // if previous has an on, ignore
				valve_mf_on = true;
				valve_mf_starttime = st;
			}

		} else if ((exp_no == 16) || (exp_no == 111)) {
			if (valve_slowc_on) {
				for (int j=valve_slowc_starttime; j<st; j++) {
					// record Cresc Valve information for previous
					isSlowC->at(j) = true;
				}
			}
			valve_slowc_on = false;

		} else if ((exp_no == 17) || (exp_no == 110)) { // Crescendo on (slow)
			if (!valve_slowc_on) { // if previous has an on, ignore
				valve_slowc_on = true;
				valve_slowc_starttime = st;
			}

		// Fast Crescendo/Decrescendo is a direct operation (length of perforation matters)
		} else if ((exp_no == 18) || (exp_no == 109)) { // Forzando off -- Fast Decrescendo
				for (int j=st; j<et; j++) {
					isFastD->at(j) = true;
				}

		} else if ((exp_no == 19) || (exp_no == 108)) { // Forzando on -- Fast Crescendo
				for (int j=st; j<et; j++) {
					isFastC->at(j) = true;
				}
		}
	}


	// TODO: deal with the last case (if crescendo OFF is missing)


	// Second pass, update the current velocity according to the previous one

	double amount = 0.0;

	for (int i=1; i<exp_length; i++) {
		if ((isSlowC->at(i) == false) && (isFastC->at(i) == false) && (isFastD->at(i) == false)) {
			amount = -slow_step; // slow decrescendo is always on

		// if both slow crescendo and fast crescendo
		//elif isSlowC->at(i) == 1 and isFastC->at(i) == 1:
		//    amount = self.slow_step + self.fastC_step

		} else {
			amount = isSlowC->at(i) * slow_step + isFastC->at(i) * fastC_step + isFastD->at(i) * fastD_step;
		}

		double newV = expression_list->at(i-1) + amount;
		expression_list->at(i) = newV;

		if (isMF[i]) {
			if ((expression_list->at(i) > welte_mf) && (amount > 0)) {
				// do nothing
			} else if ((expression_list->at(i) > welte_mf) && (amount < 0)) {
				expression_list->at(i) = std::max(welte_mf, expression_list->at(i));
			} else if ((expression_list->at(i) < welte_mf) && (amount < 0)) {
				// do nothing
			} else if ((expression_list->at(i) < welte_mf) && (amount > 0)) {
				expression_list->at(i) = std::min(welte_mf, expression_list->at(i));
			}
		} else {
			// new: adding loud
			// slow crescendo will only reach welte_loud
			if (isSlowC->at(i) && (isFastC->at(i) == false)) {
				expression_list->at(i) = min(expression_list->at(i), welte_loud);
			}
		}

		expression_list->at(i) = max(welte_p, expression_list->at(i));

		// regulating max
		expression_list->at(i) = min(welte_f, expression_list->at(i));
	}
}



//////////////////////////////
//
// Expressionizer::printExpression --
//

ostream& Expressionizer::printExpression(ostream& out, bool extended) {
	int maxi = std::min(exp_bass.size(), exp_treble.size());
	for (int i=0; i<maxi; i++) {
		out << i << "\t" << exp_bass[i];
		if (extended) {
			out << "\t" << isSlowC_bass[i];
			out << "\t" << isFastC_bass[i];
			out << "\t" << isFastD_bass[i];
		}
		out << "\t" << exp_treble[i];
		if (extended) {
			out << "\t" << isSlowC_bass[i];
			out << "\t" << isFastC_bass[i];
			out << "\t" << isFastD_bass[i];
		}
		out << endl;
	}
	return out;
}



//////////////////////////////
//
// Expressionizer::applyTrackBarWidthCorrection --
//

bool Expressionizer::applyTrackBarWidthCorrection(void) {
	if (midi_data.getTrackCount() < 2) {
		cerr << "Error: no MIDI data to correct" << endl;
		return false;
	}
	if (trackbar_correction_done) {
		cerr << "Error: you already did the correction, so not doing it again." << endl;
		return false;
	}

	int correction = int(getPunchDiameter() * getPunchExtensionFraction() + 0.5);
	for (int i=0; i<midi_data.getTrackCount(); i++) {
		for (int j=0; j<midi_data[i].getEventCount(); j++) {
			if (!midi_data[i][j].isNoteOff()) {
				continue;
			}
			midi_data[i][j].tick += correction;
		}
	}

	midi_data.sortTracks();
   updateMidiTimingInfo();
	trackbar_correction_done = true;
	return true;
}



///////////////////////////////
//
// Expressionizer::setPunchDiameter --
//

void Expressionizer::setPunchDiameter(double value) {
	punch_width = value;
}



///////////////////////////////
//
// Expressionizer::setPunchExtensionFraction --
//
void Expressionizer::setPunchExtensionFraction(double value) {
	punch_fraction = value;
}



///////////////////////////////
//
// Expressionizer::getPunchDiameter --
//
double Expressionizer::getPunchDiameter(void) {
	return punch_width;
}



///////////////////////////////
//
// Expressionizer::getPunchExtensionFraction --
//

double Expressionizer::getPunchExtensionFraction(void) {
	return punch_fraction;
}



///////////////////////////////
//
// Expressionizer::removeExpressionTracksOnWrite --
//

void Expressionizer::removeExpressionTracksOnWrite(void) {
	delete_expresison_tracks = true;
}



///////////////////////////////
//
// Expressionizer::setPianoTimbre -- Add a piano patch change
//   to the start of tracks 1 and 2 (or update it to piano if
//   there already is a patch change anywhere in the track).
//

bool Expressionizer::setPianoTimbre(void) {
	MidiEvent* timbre1 = NULL;
	MidiEvent* timbre2 = NULL;
	for (int i=0; i<midi_data[1].getEventCount(); i++) {
		if (midi_data[1][i].isTimbre()) {
			timbre1 = &midi_data[1][i];
		}
	}

	for (int i=0; i<midi_data[2].getEventCount(); i++) {
		if (midi_data[2][i].isTimbre()) {
			timbre2 = &midi_data[2][i];
		}
	}

	if (midi_data.getTrackCount() < 3) {
		cerr << "Error: not enough tracks in MIDI file: " << midi_data.getTrackCount() << endl;
		return false;
	}

	bool sortQ = false;
	int track;
	int channel;
	int tick = 0;
	int timbre = 0;
	if (timbre1) {
		timbre1->setP2(timbre);
	} else {
		track = 1;
		channel = 1;
		midi_data.addTimbre(track, tick, channel, timbre);
		sortQ = true;
	}

	if (timbre2) {
		timbre2->setP2(timbre);
	} else {
		track = 2;
		channel = 2;
		midi_data.addTimbre(track, tick, channel, timbre);
		sortQ = true;
	}

	if (sortQ) {
		midi_data.sortTracks();
	}

	return true;
}



