// vim: ts=3

//    vel_mid: middle velocity (what velocity value maps to an MF)
//    dynamic_range: default 1.2, mapping maximum 96, and minimum 66
//    left_adjust: how much less you want left hand to be, default 15
//    //time_scale: speed adjustment, default 1, smaller means faster (new duration = old duration * time_scale)
//    name_ext: the extention name, default .mid
//    pan_bass: how much of the panning to the left, default 52
//    pan_treble: how much of the panning to the right, default 76


#include <string>
#include <vector>

#include "MidiFile.h"

class ExpCreator {

	public:
		void        setMidiName(std::string name) { midi_name = name; };
		void        setOutPath(std::string path) { out_path = path; };
		void        setNameExtension(std::string ext) { name_ext = ext; };
		void        setSourceDirectory(std::string dir) { src_directory = dir; };

		std::string getMidiName(void) { return midi_name; };
		std::string getOutPath(void) { return out_path; };
		std::string getNameExtension(void) { return name_ext; };
		std::string getSourceDirectory(void) { return src_directory; };

		bool readMidiFile(std::string filename);
		bool writeMidiFile(std::string filename);

		void addExpression(void);
		void setPan(void);
		void calculateExpression(void);
		void addVelocity(bool leftQ = false);
		void calculateVelocity(std::string option);
		std::ostream& printExpression(std::ostream& out);

	protected:
		double getPreviousNonzero(std::vector<double>& myArray, int start_index);

	private:
		std::string src_directory = ".";
		std::string midi_name     = "";
		std::string out_path      = ".";
		std::string name_ext      = "_exp";

		double welte_mf           = 60.0;
		double welte_f            = 85.0;
		double welte_p            = 38.0;
		double welte_loud         = 70.0;
		double cresc_rate         = 1.0;
		int left_adjust           = -15;  // reduce loudness of bass register (for attack velocities)
		double time_scale         = 1.0;
		int    pan_bass           = 52;
		int    pan_treble         = 76;
		bool   read_pedal         = true;

		smf::MidiFile midi_data;

		int bass       = 1; // MIDI track index for bass notes
		int treble     = 2; // MIDI track index for treble notes
		int bass_exp   = 3; // MIDI track index for bass notes expression
		int treble_exp = 4; // MIDI track index for treble notes expression

		int bass_ch       = 1;
		int treble_ch     = 2;
		int bass_exp_ch   = 3;
		int treble_exp_ch = 4;

		double slow_step  = -1000;
  		double fastC_step = -1000;
  		double fastD_step = -1000;

		std::vector<double> exp_bass;
		std::vector<double> exp_treble;

};



