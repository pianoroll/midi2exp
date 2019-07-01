//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp (translation to C++)
// Creation Date: Thu Oct  4 16:32:27 PDT 2018
// Last Modified: Fri Mar 22 16:25:41 PDT 2019
// Filename:      midi2exp/src/Expressionizer.cpp
// Website:       https://github.com/pianoroll/midi2exp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// description:   Class that applies expression note velocities.
//

#include "Expressionizer.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>

using namespace std;
using namespace smf;


//////////////////////////////
//
// Expressionizer::Expressionizer -- Constructor.
//

Expressionizer::Expressionizer(void) {
	setupRedWelte();  // default setup is for Red Welte rolls.
}



//////////////////////////////
//
// Expressionizer::setupRedWelte -- Prepare constants for Red Welte rolls.
//

void Expressionizer::setupRedWelte(void) {
	slowC_step  =   (welte_mf - welte_p) / slow_cresc_rate;
	slowD_step  = - (welte_mf - welte_p) / slow_decay_rate;
	fastC_step  =   (welte_mf - welte_p) / fastC_decay_rate;
	fastD_step  = - (welte_f - welte_p)  / fastD_decay_rate;
	// cerr << "CRESC_RATE " << cresc_rate << endl;
	// cerr << "SLOW STEP " << slow_step << endl;
	// cerr << "FASTC STEP " << fastC_step << endl;
	// cerr << "FASTD STEP " << fastD_step << endl;

	// expression keys for Red Welte rolls:
	PedalOnKey     = 106;
	PedalOffKey    = 107;
	SoftOnKey      = 21;
	SoftOffKey     = 20;
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

	midi_data.applyAcceleration(12, 0.22);
	calculateRedWelteExpression("left_hand");
	calculateRedWelteExpression("right_hand");

	applyExpression("left_hand");
	applyExpression("right_hand");

	addSustainPedalling(treble_exp_track, PedalOnKey, PedalOffKey);
	addSoftPedalling(bass_exp_track, SoftOnKey, SoftOffKey);
}



//////////////////////////////
//
// Expressionizer::addSustainPedalling -- Extract sustain pedal information
//    from the expression track and insert sustain pedalling into the
//    bass and treble note channels/tracks.
//

void Expressionizer::addSustainPedalling(int sourcetrack, int onkey, int offkey) {
	MidiRoll& midifile = midi_data;
	const int pedal_controller = 64;  // sustain pedal

	bool hascontroller = hasControllerInTrack(bass_track, pedal_controller);
	if (hascontroller) {
		cerr << "Warning: Bass track already contains sustain pedalling." << endl;
		return;
	}

	hascontroller = hasControllerInTrack(treble_track, pedal_controller);
	if (hascontroller) {
		cerr << "Warning: Treble track already contains sustain pedalling." << endl;
		return;
	}

	// Search through the track for notes that represent pedal on or pedal off.
	// The sustain pedal needs to be duplicated to be added to both the bass
	// track/channel and the treble track/channel.
	int tick;
	MidiEventList& events = midifile[sourcetrack];
	int count = events.getEventCount();
	for (int i=0; i<count; i++) {
		if (!events[i].isNoteOn()) {
			continue;
		}
		int key = events[i].getKeyNumber();
		tick = events[i].tick;
		if (key == onkey) {
			midifile.addController(bass_track,   tick, bass_ch,   pedal_controller, 127);
			midifile.addController(treble_track, tick, treble_ch, pedal_controller, 127);
		} else if (key == offkey) {
			midifile.addController(bass_track,   tick, bass_ch,   pedal_controller, 0);
			midifile.addController(treble_track, tick, treble_ch, pedal_controller, 0);
		}
	}
	midifile.sortTracks();
}



//////////////////////////////
//
// Expressionizer::addSoftPedalling --
//

void Expressionizer::addSoftPedalling(int sourcetrack, int onkey, int offkey) {
	MidiRoll& midifile = midi_data;
	const int pedal_controller = 67;  // soft pedal

	bool hascontroller = hasControllerInTrack(bass_track, pedal_controller);
	if (hascontroller) {
		cerr << "Warning: Bass track already contains soft pedalling." << endl;
		return;
	}

	hascontroller = hasControllerInTrack(treble_track, pedal_controller);
	if (hascontroller) {
		cerr << "Warning: Treble track already contains soft pedalling." << endl;
		return;
	}

	int tick;
	MidiEventList& events = midifile[sourcetrack];
	int count = events.getEventCount();
	for (int i=0; i<count; i++) {
		if (!events[i].isNoteOn()) {
			continue;
		}
		int key = events[i].getKeyNumber();
		tick = events[i].tick;
		if (key == SoftOnKey) {
			midifile.addController(bass_track,   tick, bass_ch,   pedal_controller, 127);
			midifile.addController(treble_track, tick, treble_ch, pedal_controller, 127);
		} else if (key == SoftOffKey) {
			midifile.addController(bass_track,   tick, bass_ch,   pedal_controller, 0);
			midifile.addController(treble_track, tick, treble_ch, pedal_controller, 0);
		}
	}
	midifile.sortTracks();
}



//////////////////////////////
//
// Expressionizer::hasControllerInTrack -- Returns true if there is a MIDI
//    message in the given track number that is a controller message, where
//    the "controller" parameter is the controller number (such as 64 which
//    is the General MIDI controller number for sustain pedal).
//

bool Expressionizer::hasControllerInTrack(int track, int controller) {
	MidiEventList& events = midi_data[track];
	int count = events.getEventCount();
	MidiEvent* me;
	for (int i=0; i<count; i++) {
		me = &events[i];
		if (!me->isController()) {
			continue;
		}
		if (me->getP1() == controller) {
			return true;
		}
	}
	return false;
}



//////////////////////////////
//
// Expressionizer::setRollTempo -- Set the tempo of the roll
// input: tempo  (slow: 67.25, regular: 104.331, fast: 143.373)
// (old tempo: 98.4252 for red welte), (default 300 dpi, 6 as multiplier)
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

	addMetadata();

	return midi_data.write(filename);
}


//////////////////////////////
//
// Expressionizer::addMetadata --
//

void Expressionizer::addMetadata(void) {
	midi_data.setMetadata("EXP_SOFTWARE", "\t\thttps://github.com/pianoroll/midi2exp");

	stringstream ss;
	ss << "@EXP_SOFTWARE_DATE:\t" << __DATE__ << " " << __TIME__   << "";
	string sss = ss.str();
	sss = ss.str();
	sss.erase(remove(sss.begin(), sss.end(), '\n'), sss.end());
	midi_data.addText(0, 0, sss);

	ss.str("");
	std::chrono::system_clock::time_point nowtime = std::chrono::system_clock::now();
	std::time_t current_time = std::chrono::system_clock::to_time_t(nowtime);
	ss << "@EXP_DATE:\t\t"     << std::ctime(&current_time);
	sss = ss.str();
	sss.erase(remove(sss.begin(), sss.end(), '\n'), sss.end());
	midi_data.addText(0, 0, sss);

	if (trackbar_correction_done) {
		int correction = int(getTrackerbarDiameter() * getPunchExtensionFraction() + 0.5);
		string value = "\t" + to_string(correction) + "px";
		midi_data.setMetadata("TRACKER_EXTENSION", value);
	} else {
		midi_data.setMetadata("TRACKER_EXTENSION", "\t0px");
	}

	ss.str("");
	ss << "\t\t" << getWelteP();
	midi_data.setMetadata("EXP_WELTE_P", ss.str());

	ss.str("");
	ss << "\t\t" << getWelteMF();
	midi_data.setMetadata("EXP_WELTE_MF", ss.str());

	ss.str("");
	ss << "\t\t" << getWelteF();
	midi_data.setMetadata("EXP_WELTE_F", ss.str());

	ss.str("");
	ss << "\t" << getWelteLoud();
	midi_data.setMetadata("EXP_WELTE_LOUD", ss.str());

	ss.str("");
	ss << "\t" << getLeftRightDiff();
	midi_data.setMetadata("LEFT_HAND_ADJUST", ss.str());

	ss.str("");
	ss << "\t" << getSlowDecayRate() << " ms (time from welte_p to welte_mf)";
	midi_data.setMetadata("EXP_WELTE_SLOW_DECAY", ss.str());

	ss.str("");
	ss << "\t" << getFastCrescendo() << " ms (time from welte_p to welte_mf)";
	midi_data.setMetadata("EXP_WELTE_FAST_CRES", ss.str());

	ss.str("");
	ss << "\t" << getFastDecrescendo() << " ms (time from welte_p to welte_f)";
	midi_data.setMetadata("EXP_WELTE_FAST_DECRS", ss.str());

	midi_data.setMetadata("MIDIFILE_TYPE", "\texp");

}




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

		// if still equals 0, map it to minimum velocity
		if (velocity == 0) {
			velocity = welte_p;
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
	vector<double>* isMF;
	vector<double>* isSlowC;
	vector<double>* isSlowD;
	vector<double>* isFastC;
	vector<double>* isFastD;

	// Initial Setup of the expression curve
	if (option == "left_hand") {
		track_index = bass_exp_track;
		expression_list = &exp_bass;
		isMF = &isMF_bass;
		isSlowC = &isSlowC_bass;
		isSlowD = &isSlowD_bass;
		isFastC = &isFastC_bass;
		isFastD = &isFastD_bass;
	} else {
		track_index = treble_exp_track;
		expression_list = &exp_treble;
		isMF = &isMF_treble;
		isSlowC = &isSlowC_treble;
		isSlowD = &isSlowD_treble;
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

	// vector<bool> isMF(exp_length, false);    // setting up the upper/lower bound

	isMF->resize(exp_length);
	isSlowC->resize(exp_length);
	isSlowD->resize(exp_length);
	isFastC->resize(exp_length);
	isFastD->resize(exp_length);
	std::fill(isMF->begin(), isMF->end(), false);        // is MF hook on?
	std::fill(isSlowC->begin(), isSlowC->end(), false);  // is slow crescendo on?
	std::fill(isSlowD->begin(), isSlowC->end(), true);  // is slow crescendo on?
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
		// if (option == "right_hand"){
		// 	printf("i: %d\t", i);
		// 	printf("exp_no: %d\n", exp_no);
		// }

		if ((exp_no == 14) || (exp_no == 113)) {
			// MF off
			if (valve_mf_on) {
				for (int j=valve_mf_starttime; j<st; j++) {
					// record MF Valve information for previous
					isMF->at(j) = true;
					//cout << "MF is on" << '\t' << j << endl;
				}
			}
			valve_mf_on = false;

		} else if ((exp_no == 15) || (exp_no == 112)) {
			// MF on, just update the start Time
			if (!valve_mf_on) {    // if previous has an on, ignore
				valve_mf_on = true;
				valve_mf_starttime = st;
				// if (option == "right_hand"){
				// 	printf("detect MF on, recording valve_mf_starttime = %d\n", valve_mf_starttime);
				// }
			}
		}
		// detect slow crescendo on, update starttime
		  else if ((exp_no == 17) || (exp_no == 110)) { // Crescendo on (slow)
			if (!valve_slowc_on) { // if previous has an on, ignore
				valve_slowc_on = true;
				valve_slowc_starttime = st;
				// if (option == "right_hand"){
				// 	printf("detect slowC on, recording valve_slowc_starttime = %d\n", valve_slowc_starttime);
				// }
			}
		}
		// detect slow decrescendo off, update isSlowC
		  else if ((exp_no == 16) || (exp_no == 111)) {
			if (valve_slowc_on) {
				for (int j=valve_slowc_starttime; j<st; j++) {
					// record Cresc Valve information for previous
					isSlowC->at(j) = true;
				}
				// if (option == "right_hand"){
				// 	printf("update isSlowC from %d\t", valve_slowc_starttime);
				// 	printf("to %d\n", st-1);
				// }
			}
			valve_slowc_on = false;
		}
		// Fast Crescendo/Decrescendo is a direct operation (length of perforation matters)
		  else if ((exp_no == 18) || (exp_no == 109)) { // Forzando off -- Fast Decrescendo
				for (int j=st; j<et; j++) {
					isFastD->at(j) = true;
				}
				// if (option == "left_hand"){
				// 	printf("Fast Decrescendo on from %d\t", st);
				// 	printf("to %d\n", et);
				// }

		} else if ((exp_no == 19) || (exp_no == 108)) { // Forzando on -- Fast Crescendo
				for (int j=st; j<et; j++) {
					isFastC->at(j) = true;
				}
				// if (option == "left_hand"){
				// 	printf("Fast Crecrescendo on from %d\t", st);
				// 	printf("to %d\n", et);
				// }
		}
	}

	// TODO: deal with the last case (if crescendo OFF is missing)

	// Second pass, update the current velocity according to the previous one

	double amount = 0.0;
	double eps = 0.0001;
	bool isSlowC_first = false;
    int slowC_t0 = 0;
    double slowC_sum = 0.0;

	for (int i=1; i<exp_length; i++) {
		// linear method, always decrescendo on
		amount = slowD_step + isSlowC->at(i) * slowC_step + isFastC->at(i) * fastC_step + isFastD->at(i) * fastD_step;

		// compute the amount using non-linear fitting for slow-crescendo
		// if ((isSlowC->at(i) == false) && (isFastC->at(i) == false) && (isFastD->at(i) == false)) {
		// 	amount = -slow_step;  // slow_decresc_rate + 500
  //           slowC_sum = 0.0;
  //           slowC_t0 = i;
		// } else {
  //           amount = isFastC->at(i) * fastC_step + isFastD->at(i) * fastD_step;
            //cout << "amount: " << amount << endl;
            //amount += slow_step;
            // cout << "amount without slowC: " << amount << endl;
        //     if (isSlowC->at(i)){
        //     	// cout << "isSlowC on at " << i << endl;
        //         if (isSlowC_first == false){
        //             isSlowC_first = true;
        //             // cout << "isSlowC_first = true, slowC_t0 = " << i << endl;
        //             slowC_t0 = i;
        //             slowC_sum = 0.0;
        //         }
        //         // new_t = i - slow_t0
        //         // amount += f(new_t) - f(new_t-1)
        //         //double aaa = sc_B * log(sc_C + i-slowC_t0);

        //         if (i >= slowC_t0) {
        //         	// =67.62765+(34.79438-67.62765)/(1+(L1/1861.557)^4.869496)
        //         	double after  = (35.30282-69.40928)/(1 + pow((i-slowC_t0  ) / 1202.475, 2.808176));
        //         	double before = (35.30282-69.40928)/(1 + pow((i-slowC_t0-1) / 1202.475, 2.808176));
        //         	double aaa = after - before;
        //         	// double aaa = sc_B * log(sc_C + i-slowC_t0) - sc_B * log(sc_C + i-slowC_t0-1) ;
        //         	// using first order approximation
        //         	//double aaa = sc_B / ((i-slowC_t0-1.0)+ 500.0);
        //         	amount += aaa;
        //         	slowC_sum += aaa;
        //         	//cout << "new_t: " << (i-slowC_t0) << " aaa: " << aaa << " sum: " << slowC_sum <<  " i: " << i << " slowC_t0: " << slowC_t0 << endl;
        //         }
        //     }
        //     else{
        //         isSlowC_first = false;
        //         slowC_sum = 0.0;
        //         slowC_t0 = i;
        //     }
        // }

		double newV = expression_list->at(i-1) + amount;
		//cout << ("newV: " + std::to_string(newV)) << endl;
		// if (option == "right_hand"){
		// 	printf("newV: %f\t", newV);
		// }
		expression_list->at(i) = newV;

      //   if (i > 2900 && i < 3200) {
	    	// cout << "i: " << i << " amount: " << amount << " newV:" << newV << endl;
      //   }


        // clipping
		if (isMF->at(i) == true) {
			if (expression_list->at(i-1) > welte_mf){
				if (amount < 0) {
					expression_list->at(i) = std::max(welte_mf + eps, expression_list->at(i));
				}
				//cout << "taking max" << endl;
				else{
					expression_list->at(i) = std::min(welte_f, expression_list->at(i));
				}
			}
			else if (expression_list->at(i-1) < welte_mf){
				if (amount > 0){
					expression_list->at(i) = std::min(welte_mf - eps, expression_list->at(i));
				}
				else{
					expression_list->at(i) = std::max(welte_p, expression_list->at(i));
				}
				//cout << " taking min " << endl;
			}
			else{
				//cout << "equals 60 here" << endl;
			}
			// if ((expression_list->at(i-1) > welte_mf) && (amount > 0)) {
			// 	// do nothing
			// } else if ((expression_list->at(i-1) >= welte_mf) && (amount < 0)) {
			// 	expression_list->at(i) = std::max(welte_mf, expression_list->at(i));
			// } else if ((expression_list->at(i-1) < welte_mf) && (amount < 0)) {
			// 	// do nothing
			// } else if ((expression_list->at(i-1) <= welte_mf) && (amount > 0)) {
			// 	expression_list->at(i) = std::min(welte_mf, expression_list->at(i));
			// }
		} else {
			// slow crescendo will only reach welte_loud
			if (isSlowC->at(i) && (isFastC->at(i) == false) && expression_list->at(i-1) < welte_loud) {
				expression_list->at(i) = min(expression_list->at(i), welte_loud - eps);
			}
		}
		// regulating max and min
		expression_list->at(i) = max(welte_p, expression_list->at(i));
		expression_list->at(i) = min(welte_f, expression_list->at(i));
		//cout << ("newV after process: " + std::to_string(expression_list->at(i))) << endl;
		// if (option == "right_hand"){
		// 	printf("newV after process: %f\n", expression_list->at(i));
		// }
		//cout << expression_list->at(i) << endl;
	}

	//convert from windchest pressure to midi velocity using nonlinear function
	// for (int i=0; i<exp_length; i++) {
	// 	expression_list->at(i) = -155 + 54.3 * log(expression_list->at(i));
	// }
	//convert from windchest pressure to midi velocity using nonlinear function
	for (int i=0; i<exp_length; i++) {
		expression_list->at(i) = -151 + 52 * log(expression_list->at(i));
	}

}


//////////////////////////////
//
// Expressionizer::printVelocity --
//
void Expressionizer::printVelocity() {
	ofstream outFile("vel-bass.txt");
	for (const auto &e : exp_bass) outFile << e << "\n";
	ofstream outFile2("vel-treble.txt");
	for (const auto &e2 : exp_treble) outFile2 << e2 << "\n";
	ofstream outFile3("mf-bass.txt");
	for (const auto &e3 : isMF_bass) outFile3 << e3 << "\n";
	ofstream outFile4("mf-treble.txt");
	for (const auto &e4 : isMF_treble) outFile4 << e4 << "\n";
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
			out << "\t" << isSlowD_bass[i];
			out << "\t" << isFastC_bass[i];
			out << "\t" << isFastD_bass[i];
		}
		out << "\t" << exp_treble[i];
		if (extended) {
			out << "\t" << isSlowC_treble[i];
			out << "\t" << isSlowD_treble[i];
			out << "\t" << isFastC_treble[i];
			out << "\t" << isFastD_treble[i];
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

	int correction = int(getTrackerbarDiameter() * getPunchExtensionFraction() + 0.5);
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
// Expressionizer::setTrackerbarDiameter --
//

void Expressionizer::setTrackerbarDiameter(double value) {
	tracker_width = value;
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
// Expressionizer::getTrackerbarDiameter --
//
double Expressionizer::getTrackerbarDiameter(void) {
	return tracker_width;
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



///////////////////////////////
//
// Expressionizer::setWelteP -- Set the dynamic range for Welte (for p dynamic)
//

void Expressionizer::setWelteP(double value) {
	welte_p = value;
}



///////////////////////////////
//
// Expressionizer::setWelteMF -- Set the dynamic range for Welte (for mf dynamic)
//

void Expressionizer::setWelteMF(double value) {
	welte_mf = value;

}



///////////////////////////////
//
// Expressionizer::setWelteF -- Set the dynamic range for Welte (for f dynamic)
//

void Expressionizer::setWelteF(double value) {
	welte_f = value;
}



///////////////////////////////
//
// Expressionizer::setWelteLoud -- Set the dynamic range for Welte
//

void Expressionizer::setWelteLoud(double value) {
	welte_loud = value;
}


///////////////////////////////
//
// Expressionizer::setSlowCrescRate -- Set expresion information for red Welte rolls.
//

void Expressionizer::setSlowCrescRate(double value) {
	//slow_step = value;
	slow_cresc_rate = value;
}


///////////////////////////////
//
// Expressionizer::setSlowDecayRate -- Set expresion information for red Welte rolls.
//

void Expressionizer::setSlowDecayRate(double value) {
	//slow_step = value;
	slow_decay_rate = value;
}



///////////////////////////////
//
// Expressionizer::setFastCrescendo -- Set expresion information for red Welte rolls.he dynamic range for Welte
//

void Expressionizer::setFastCrescendo(double value) {
	//fastC_step = value;
	fastC_decay_rate = value;
}



///////////////////////////////
//
// Expressionizer::setFastDecrescendo -- Set expresion information for red Welte rolls.the dynamic range for Welte
//
void Expressionizer::setFastDecrescendo(double value) {
	//fastD_step = value;
	fastD_decay_rate = value;
}



double Expressionizer::getWelteP(void)          { return welte_p;    }
double Expressionizer::getWelteMF(void)         { return welte_mf;   }
double Expressionizer::getWelteF(void)          { return welte_f;    }
double Expressionizer::getWelteLoud(void)       { return welte_loud; }
double Expressionizer::getSlowDecayRate(void)   { return slow_decay_rate;  }
double Expressionizer::getFastCrescendo(void)   { return fastC_decay_rate; }
double Expressionizer::getFastDecrescendo(void) { return fastD_decay_rate; }
double Expressionizer::getLeftRightDiff(void)   { return left_adjust; }
