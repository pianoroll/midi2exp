//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp (translation to C++)
// Creation Date: Thu Oct  4 16:32:27 PDT 2018
// Last Modified: Fri Oct  5 00:25:41 PDT 2018
// Filename:      midi2exp/include/Expressionizer.h
// Website:       https://github.com/pianoroll/midi2exp
// Syntax:        C++11
// vim:           ts=3 noexpandtab
//
// description:   Class that applies expression note velocities.
//
//
//    vel_mid: middle velocity (what velocity value maps to an MF)
//    dynamic_range: default 1.2, mapping maximum 96, and minimum 66
//    left_adjust: how much less you want left hand to be, default 15
//    time_scale: speed adjustment, default 1, smaller means faster
//    (new duration = old duration * time_scale)
//    name_ext: the extention name, default .mid
//    pan_bass: how much of the panning to the left, default 52
//    pan_treble: how much of the panning to the right, default 76
//

#ifndef _EXPRESSIONIZER_H_INCLUDED
#define _EXPRESSIONIZER_H_INCLUDED

#include <string>
#include <vector>

#include "MidiFile.h"

class Expressionizer {

	public:
		              Expressionizer               (void);
		             ~Expressionizer               (void);

		bool          readMidiFile                 (std::string filename);
		bool          writeMidiFile                (std::string filename);

		std::ostream& printExpression              (std::ostream& out, bool extended = false);

		void          addExpression                (void);
		void          setPan                       (void);

		bool          applyTrackBarWidthCorrection (void);
		void          updateMidiTimingInfo         (void);

		void          setRollTempo	               (double value);
		void          setPunchDiameter             (double value);
		void          setPunchExtensionFraction    (double value);
		double        getPunchDiameter             (void);
		double        getPunchExtensionFraction    (void);
		void          removeExpressionTracksOnWrite(void);
		bool          setPianoTimbre               (void);

		void    addSustainPedalling (MidiFile& midifile, int sourcetrack,
                             int targettrack);
		void    addSoftPedalling    (MidiFile& midifile, int sourcetrack,
                             int targettrack);

	protected:
		void          calculateRedWelteExpression (std::string option);
		void          applyExpression             (std::string option);
		double        getPreviousNonzero          (std::vector<double>& myArray,
		                                           int start_index);
	private:

		double welte_p            = 38.0;
		double welte_mf           = 60.0;
		double welte_f            = 85.0;
		double welte_loud         = 70.0;
		double cresc_rate         = 1.0;

		double punch_width         = 21.5;  // diameter of the hole punches (in pixels/ticks)
		double punch_fraction      = 0.25;  // extenion length of holes (0.25 = 25% longer)

		bool trackbar_correction_done = false;
		bool delete_expresison_tracks = false;

		// left_adjust: reduce loudness of bass register (for attack velocities)
		int left_adjust           = -10;

		double time_scale         = 1.0;

		// pan_bass: the MIDI pan controller value for bass register:
		int    pan_bass           = 52;

		// pan_treble: the MIDI pan controller value for treble register:
		int    pan_treble         = 76;

		int    TREBLE_EXPRESSION  = 4;
		int    BASS_EXPRESSION    = 3;

		bool   read_pedal         = true;

		// midi_data: store of the input/output MIDI data file:
		smf::MidiFile midi_data;

		// MIDI tracks for each component of the input MIDI data.
		// The 0th track is for tempo meta messages (no notes):
		int bass_track       = 1; // track index for bass register notes
		int treble_track     = 2; // track index for treble register notes
		int bass_exp_track   = 3; // track index for bass notes expression
		int treble_exp_track = 4; // track index for treble notes expression

		// MIDI channels for each component of the input MIDI data:
		int bass_ch          = 1; // channel number of bass register notes
		int treble_ch        = 2; // channel number of treble register notes
		int bass_exp_ch      = 3; // channel number of bass notes expression
		int treble_exp_ch    = 4; // channel number of treble notes expression

		double slow_step     = -1000;
  		double fastC_step    = -1000;
  		double fastD_step    = -1000;

		// exp_bass: the model expression at every millisecond for bass register.
		std::vector<double> exp_bass;
		std::vector<double> isSlowC_bass;
		std::vector<double> isFastC_bass;
		std::vector<double> isFastD_bass;

		// exp_treble: model expression at every millisecond for treble register.
		std::vector<double> exp_treble;
		std::vector<double> isSlowC_treble;
		std::vector<double> isFastC_treble;
		std::vector<double> isFastD_treble;

  		// 2.0 is some regulation, modified from table 4.1 from Peter's thesis
		double slow_decay_rate  = 2380.0;
		double fastC_decay_rate = 700.0 * 2.0;
		double fastD_decay_rate = 150.0 * 2.2;
};


#endif /* _EXPRESSIONIZER_H_INCLUDED */

