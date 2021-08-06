//
// Programmer:    Kitty Shi
// Programmer:    Craig Stuart Sapp (translation to C++)
// Creation Date: Thu Oct  4 16:32:27 PDT 2018
// Last Modified: Wed Aug  4 15:36:22 PST 2021
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

#include "MidiRoll.h"

class Expressionizer {

	public:
		              Expressionizer               (void);
		             ~Expressionizer               (void);

		bool          readMidiFile                 (std::string filename);
		bool          writeMidiFile                (std::string filename);

		std::ostream& printExpression              (std::ostream& out, bool extended = false);
		void 		     printVelocity();

		void          addExpression                (void);
		void          setPan                       (void);

		bool          applyTrackBarWidthCorrection (void);
		void          updateMidiTimingInfo         (void);

		void          setRollTempo                 (double value);
		void          setPunchDiameter             (double value);
		void          setTrackerbarDiameter        (double value);

		void          setPunchExtensionFraction    (double value);
		double        getPunchDiameter             (void);
		double		  getTrackerbarDiameter        (void);
		double        getPunchExtensionFraction    (void);
		void          removeExpressionTracksOnWrite(void);
		bool          setPianoTimbre               (void);

		void          addSustainPedallingLockAndCancel (int sourcetrack, int onkey, int offkey);
		void          addSoftPedallingLockAndCancel    (int sourcetrack, int onkey, int offkey);
		void          addSustainPedalling          (int sourcetrack, int onkey);
		void          addSoftPedalling             (int sourcetrack, int onkey);

		void          setupRedWelte                (void);
		void          setupLicenseeWelte           (void);
		void          setupGreenWelte              (void);
		void          setup88Roll                  (void);

		void          setWelteP                    (double value);
		void          setWelteMF                   (double value);
		void          setWelteF                    (double value);
		void          setWelteLoud                 (double value);
		void          setSlowDecayRate             (double value);
		void          setFastCrescendo             (double value);
		void          setFastDecrescendo           (double value);
		void          setVersion                   (std::string version);

		double        getWelteP                    (void);
		double        getWelteMF                   (void);
		double        getWelteF                    (void);
		double        getWelteLoud                 (void);
		double        getSlowDecayRate             (void);
		double        getFastCrescendo             (void);
		double        getFastDecrescendo           (void);
		double        getLeftRightDiff             (void);

		void          setAcceleration              (double inches, double percent);


	protected:
		void          addMetadata                     (void);
		bool          hasControllerInTrack            (int track, int controller);
		void          calculateRedWelteExpression     (const std::string& option);
		std::vector<double>*          calculateLicenseeWelteExpression(const std::string& option);
		void          calculateGreenWelteExpression   (const std::string& option);
		void          calculate88Expression           (const std::string& option);
		void          applyExpression                 (const std::string& option);
		double        getPreviousNonzero              (std::vector<double>& myArray, int start_index);

	private:
		std::string roll_type = "red";

		double welte_p        = 35.0;  //
		double welte_mf       = 60.0;  //
		double welte_f        = 90.0;  //
		double welte_loud     = 75.0;
		double note_normal88  = 75;    // default velocity for 88 notes
		double cresc_rate     = 1.0;
		double snake_f        = 95;    // snakebite velocity
		int snake_gracetime   = 200;    // +- 200ms grace period for snakebite to take effect

		double punch_width    = 21.5;  // diameter of the hole punches (in pixels/ticks)
		double punch_fraction = 0.75;  // extention length of holes (0.75 = 75% longer)

		double tracker_width = 1.413 * 300.25 / 25.4; // diameter of the punch hole (1.413mm * 300.25 pixels/inch / 25.44 mm/inch)
		double tracker_fraction = 0.75;               // for hole extension, 75% of the tracker bar diameter

		bool trackbar_correction_done = false;
		bool delete_expresison_tracks = false;

		// version
		std::string m_version;
		double      m_inches  = 12.0;
		double      m_percent = 0.22;

		// left_adjust: reduce loudness of bass register (for attack velocities)
		int left_adjust       = -5;

		double time_scale     = 1.0;

		// pan_bass: the MIDI pan controller value for bass register:
		int    pan_bass       = 52;

		// pan_treble: the MIDI pan controller value for treble register:
		int    pan_treble     = 76;

		// expression keys for Red Welte rolls (initialized in setupRedWelte()):
		int    PedalOnKey;
		int    PedalOffKey;
		int    SoftOnKey;
		int    SoftOffKey;

		// for 88-note rolls (treble and bass)
		int    Snakebite_treble;
		int    Snakebite_bass;

		bool   read_pedal     = true;

		// midi_data: store of the input/output MIDI data file:
		smf::MidiRoll midi_data;

		// MIDI tracks for each component of the input MIDI data.
		// The 0th track is for tempo meta messages (no notes):
		int bass_track        = 1; // track index for bass register notes
		int treble_track      = 2; // track index for treble register notes
		int bass_exp_track    = 3; // track index for bass notes expression
		int treble_exp_track  = 4; // track index for treble notes expression

		// MIDI channels for each component of the input MIDI data:
		int bass_ch           = 1; // channel number of bass register notes
		int treble_ch         = 2; // channel number of treble register notes
		int bass_exp_ch       = 0; // channel number of bass notes expression
		int treble_exp_ch     = 3; // channel number of treble notes expression

		double slow_step      = -1000;
  		double fastC_step     = -1000;
  		double fastD_step     = -1000;

		// exp_bass: the model expression at every millisecond for bass register.
		std::vector<double> exp_bass;
		std::vector<double> isMF_bass;
		std::vector<double> isSlowC_bass;
		std::vector<double> isFastC_bass;
		std::vector<double> isFastD_bass;

		// exp_treble: model expression at every millisecond for treble register.
		std::vector<double> exp_treble;
		std::vector<double> isMF_treble;
		std::vector<double> isSlowC_treble;
		std::vector<double> isFastC_treble;
		std::vector<double> isFastD_treble;

  		// Regulation of crescendo and decrescendo rate
		//double slow_decay_rate  = 2380.0 * 2.0;
		//double fastC_decay_rate = 170.0;
		//double fastD_decay_rate = 150.0 * 2.2;

		// Green Welte v1 (0708):
		// double slow_decay_rate  = 2366;
		// double fastC_decay_rate = 254; //
		// double fastD_decay_rate = 269; // more than 245, more than 280 or just 189ms

		// experiment 0411
		// Red Welte v1:
		double slow_decay_rate  = 2380; //2380
		double fastC_decay_rate = 300;  // test roll shows around 170ms-200ms from min to MF hook
		double fastD_decay_rate = 400;  // test roll shows 166ms -- 300ms at max 400ms fast decrescendo can bring Max down to Min

		// before 0411
		// double slow_decay_rate  = 2380.0 * 4.0;
		// double fastC_decay_rate = 700.0; //1050.0;
		// double fastD_decay_rate = 330;
};


#endif /* _EXPRESSIONIZER_H_INCLUDED */

