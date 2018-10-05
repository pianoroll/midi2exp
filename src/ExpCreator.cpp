// vim: ts=3

#include "ExpCreator.h"

#include <algorithm>
#include <iostream>

using namespace std;
using namespace smf;


//////////////////////////////
//
// ExpCreator::addExpression -- Add expression to a MIDI file.
//

void ExpCreator::addExpression(void) {

	// calculate all expression slopes:
  	// 2.0 is some regulation, modified from table 4.1 from Peter's thesis
	double slow_decay  = 2380.0;
	double fastC_decay = 700.0 * 2.0;
	double fastD_decay = 150.0 * 2.2;

  	slow_step   =  cresc_rate * welte_mf            / slow_decay;
  	fastC_step  =  cresc_rate * (welte_f - welte_p) / fastC_decay;
  	fastD_step  = -cresc_rate * (welte_f - welte_p) / fastD_decay;

	setPan();
	calculateExpression();

}



//////////////////////////////
//
// ExpCreator::setPan -- Set a panning position for the bass and treble halfs of the piano.
// 	Note: original midi file somehow has a panning information (32 left 96 right)
//          The following function should update that message with a new value, and otherwise
//          add a new message like it is doing below.
//

void ExpCreator::setPan(void) {
	int tick = 1;
	int controller_number = 10; // 10 = pan (0-indexed)
	midi_data.addController(bass,   tick, bass_ch,   controller_number, pan_bass);
	midi_data.addController(treble, tick, treble_ch, controller_number, pan_treble);
}



//////////////////////////////
//
// ExpCreator::calculateExpression --
//

void ExpCreator::calculateExpression(void) {

	// sort expression according to time with pedaling information removed to avoid confusion
	// pedal is on notes 20, 21, 106, 107

	// then merge expression according to different combinations

	calculateVelocity("left_hand");
	calculateVelocity("right_hand");

	// Adding Velocity
	addVelocity(true);
	addVelocity(false);

	// Adding Pedal
	// if self.read_pedal is true:
	// 		self.addPedal()

}



//////////////////////////////
//
// ExpCreator::readMidiFile --
//

bool ExpCreator::readMidiFile(std::string filename) {
	return midi_data.read(filename);
}

//////////////////////////////
//
// ExpCreator::writeMidiFile --
//

bool ExpCreator::writeMidiFile(std::string filename) {
	return midi_data.write(filename);
}



/*
	def writeOut(self):
		// Timbre and output
		outMidi = pretty_midi.PrettyMIDI()
		leftHand = pretty_midi.Instrument(program=pretty_midi.instrument_name_to_program('Acoustic Grand Piano'))
		rightHand = pretty_midi.Instrument(program=pretty_midi.instrument_name_to_program('Acoustic Grand Piano'))

		outMidi.instruments.append(self.left)
		outMidi.instruments.append(self.right)

		// Write out the MIDI data
		outMidi.write(self.out_path)
		print 'write to : ', self.out_path
*/


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
// ExpCreator::addVelocity --
//

void ExpCreator::addVelocity(bool leftQ) {

	int track;

	vector<double>* myexp;

	if (leftQ) {
		track = bass;
		myexp = &exp_bass;
	} else {
		track = treble;
		myexp = &exp_treble;
	}
	MidiEventList& mynotes = midi_data[track];

	for (int i=0; i<mynotes.getEventCount(); i++) {
		MidiEvent* me = &mynotes[i];
		if (!me->isNoteOn()) {
			continue;
		}

		// find closest velocity of the begin
		int notebegin = int(me->seconds * 1000 + 0.5);
		// int noteend = int((me->seconds + me->getDurationInSecond()) * 1000 + 0.5);
		int ii = std::min(notebegin, (int)myexp->size() - 1);
		int v = int(myexp->at(ii) + 0.5);

		if (v == 0) {
			v = getPreviousNonzero(*myexp, ii);
		}

		if (leftQ) {
			v = std::max(v + left_adjust, 0);
		}

		// if still equals 0, map it to 60
		if (v == 0) {
			v = 60;
		}
		
		me->setVelocity(v);

	}

}



//////////////////////////////
//
// ExpCreator::getPreviousNonzero -- Local function to get a nonzero previous value.
//

double ExpCreator::getPreviousNonzero(vector<double>& myArray, int start_index) {
	for (int i=start_index; i>=0; i--) {
		if (myArray[i] > 0.0) {
			return myArray[i];
		}
	}
	//print '[Warning, did not find a velocity before this note, map to vel_mid (default 66)]'
	return welte_mf;
}



//////////////////////////////
//
// ExpCreator::calculateVelocity --
//
//		As of 2018-04-06, some modification (F: fast crescendo -- length of perforation)
//		(F+slow crescendo: fastest crescendo)
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

void ExpCreator::calculateVelocity(std::string option) {
	int track_index;
	vector<double>* expression_list;

	// Initial Setup of the expression curve
	if (option == "left_hand") {
		track_index = bass_exp;
		expression_list = &exp_bass;
	} else {
		track_index = treble_exp;
		expression_list = &exp_treble;
	}

	// exp_notes = notes for the expessions being processed.
	MidiEventList& exp_notes = midi_data[track_index];

	// length of the MIDI file in milliseconds (plus an extra millisecond to avoid problems):
	int exp_length = midi_data.getFileDurationInSeconds() * 1000 + 1;
	expression_list->resize(exp_length);

	// set all of the times to piano by default:
	std::fill(expression_list->begin(), expression_list->end(), welte_p);

	vector<bool> isMF(exp_length, false);    // setting up the upper/lower bound
	vector<bool> isSlowC(exp_length, false); // is slow crescendo on?
	vector<bool> isFastC(exp_length, false); // is fast crescendo on?
	vector<bool> isFastD(exp_length, false); // is fast decrescendo on?

	// Lock and Cancel
	bool valve_mf_on    = false;
	bool valve_slowc_on = false;

	int valve_mf_starttime    = 0;        // 0 for off
	int valve_slowc_starttime = 0;

	int starttime = 0;

	// First pass: For each time section calculate the current boolean state of each expression

	for (int i=0; i<exp_notes.getEventCount(); i++) {
		MidiEvent* me = &exp_notes[i];
		if (!me->isNoteOn()) {
			continue;
		}
		int exp_no = me->getKeyNumber();  // expression number
		int st = int(me->seconds * 1000.0 + 0.5);  // start time in milliseconds
		int et = int((me->seconds + me->getDurationInSeconds()) * 1000.0 + 0.5);  // end time in ms

		if ((exp_no == 14) || (exp_no == 113)) {  // MF off
			if (valve_mf_on) {
				for (int j=starttime; j<st; j++) {
					// record MF Valve information for previous
					isMF[j] = true;
				}
			}
			valve_mf_on = false;

		} else if ((exp_no == 15) || (exp_no == 112)) { // MF on, just update the start Time
			if (!valve_mf_on) {    // if previous has an on, ignore
				valve_mf_on = true;
				valve_mf_starttime = st;
			}
		
		} else if ((exp_no == 16) || (exp_no == 111)) {
			if (valve_slowc_on) {
				for (int j=starttime; j<st; j++) {
					// record Cresc Valve information for previous
					isSlowC[j] = true;
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
					isFastD[j] = true;
				}

		} else if ((exp_no == 19) || (exp_no == 108)) { // Forzando on -- Fast Crescendo
				for (int j=st; j<et; j++) {
					isFastC[j] = true;
				}
		}
	}


	// TODO: deal with the last case (if crescendo OFF is missing)


	// Second pass, update the current velocity according to the previous one

	double amount = 0.0;

	for (int i=1; i<exp_length; i++) {
		if ((isSlowC[i] == false) && (isFastC[i] == false) && (isFastD[i] == false)) {
			amount = -slow_step; // slow decrescendo is always on

		// if both slow crescendo and fast crescendo
		//elif isSlowC[i] == 1 and isFastC[i] == 1:
		//    amount = self.slow_step + self.fastC_step

		} else {
			amount = isSlowC[i] * slow_step + isFastC[i] * fastC_step + isFastD[i] * fastD_step;
		}

		double newV = expression_list->at(i-1) + amount;
		expression_list->at(i) = newV;

		if (isMF[i]) {
			// if it's increasing
			if ((expression_list->at(i-1) > welte_mf) && (amount > 0)) {
				// do nothing
			} else if ((expression_list->at(i-1) > welte_mf) && (amount < 0)) {
				expression_list->at(i) = std::max(welte_mf, expression_list->at(i));
				//print 'clipping to not less than 60'
			} else if ((expression_list->at(i-1) < welte_mf) && (amount < 0)) {
				// do nothing
			} else if ((expression_list->at(i-1) < welte_mf) && (amount > 0)) {
				expression_list->at(i) = std::min(welte_mf, expression_list->at(i));
				//print 'clipping to not greater than 60'
			}
		} else {
			// new: adding loud
			// slow crescendo will only reach welte_loud
			if (isSlowC[i] && (isFastC[i] == false)) {
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
// ExpCreator::printExpression --
//

ostream& ExpCreator::printExpression(ostream& out) {
	int maxi = std::min(exp_bass.size(), exp_treble.size());
	for (int i=0; i<maxi; i++) {
		out << exp_bass[i] << "\t" << exp_treble[i] << endl;
	}
	return out;
}




