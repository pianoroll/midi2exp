import numpy as np
import pretty_midi
import scipy.signal


class ExpCreator(object):

    """
    vel_mid: middle velocity (what velocity value maps to an MF)
    dynamic_range: default 1.2, mapping maximum 96, and minimum 66
    left_adjust: how much less you want left hand to be, default 15
    #time_scale: speed adjustment, default 1, smaller means faster (new duration = old duration * time_scale)
    nameext: the extention name, default .mid
    pan_left: how much of the panning to the left, default 52
    pan_right: how much of the panning to the right, default 76
    """

    def __init__(self,upper_folder,midi_name,out_path, nameext = '',
                Welte_MF = 60, Welte_F = 85, Welte_MF_Loud = 75,
                crescRate = 1.0, left_adjust = -15, time_scale = 1,
                pan_left = 52, pan_right = 76,readPedal = True):

        self.name = midi_name
        self.out_path = out_path + self.name

        self.midi_data = pretty_midi.PrettyMIDI(upper_folder + midi_name)
        self.Welte_MF = Welte_MF
        self.Welte_F = Welte_F
        self.Welte_MF_Loud = Welte_MF_Loud
        self.Welte_P = 38

        self.maxV = Welte_F
        self.minV = self.Welte_P
        self.LOUD = 70
        self.crescRate = crescRate
        self.readPedal = readPedal

        # get all slopes
        # 2.0 is some regulation, modified from table 4.1 from Peter's thesis
        self.slow_step = self.crescRate * self.Welte_MF/(2380.)
        self.fastC_step = self.crescRate * (self.maxV-self.minV)/(700.*2.0)
        self.fastD_step = - self.crescRate * (self.maxV-self.minV)/(150.*2.2)

        #self.slow_step = self.Welte_MF / (2380*self.crescSpeed)  # slow crescendo takes 2380 steps (ms) from 0 to mf hook (self.Welte_MF)
        #self.fastC_step = self.maxV / (700*self.crescSpeed)      # fast crescendo takes 700 steps (ms) from 0 to maximum (self.maxV? 127?)
        #self.fastD_step = -self.maxV / (150*self.crescSpeed)      # fast decrescendo takes 150 steps (ms) from maximum to 0
        # self.fastC_step = 127 / 700.      # fast crescendo takes 700 steps (ms) from 0 to maximum (self.maxV? 127?)
        # self.fastD_step = 127 / 150.      # fast decrescendo takes 150 steps (ms) from maximum to 0

        self.left_adjust = left_adjust
        self.time_scale = time_scale
        self.pan_left = pan_left
        self.pan_right = pan_right

        #self.preprocessRedTest()
        self.preprocess()
        self.getExp()
        self.writeOut()


    def preprocessRedTest(self):
        # Note: This is specially for a scan of welte mignon test roll in which the offset of note is wrong. Not applicable to other cases
        self.left = self.midi_data.instruments[0]

        self.right = self.midi_data.instruments[1]
        self.bass_exp = self.midi_data.instruments[2]
        self.treble_exp = self.midi_data.instruments[3]

        for note in self.left.notes:
            note.pitch += 1

        for note in self.right.notes:
            note.pitch += 1

        for note in self.treble_exp.notes:
            note.pitch += 1

        for note in self.bass_exp.notes:
            note.pitch -= 1

        # Panning (Center)
        # note: original midi file somehow has a panning information (32 left 96 right)
        self.left.control_changes.append(pretty_midi.ControlChange(10,self.pan_left,0.001))
        self.right.control_changes.append(pretty_midi.ControlChange(10,self.pan_right,0.001))


    def preprocess(self):
        self.left = self.midi_data.instruments[0]
        self.right = self.midi_data.instruments[1]
        self.bass_exp = self.midi_data.instruments[2]
        self.treble_exp = self.midi_data.instruments[3]

        # Panning (Center)
        # note: original midi file somehow has a panning information (32 left 96 right)
        self.left.control_changes.append(pretty_midi.ControlChange(10,self.pan_left,0.001))
        self.right.control_changes.append(pretty_midi.ControlChange(10,self.pan_right,0.001))


    def getExp(self):

        # sort expression according to time with pedaling information removed to avoid confusion
        x = [(note.start,note.end,note.pitch) for note in self.bass_exp.notes if note.pitch not in [20,21,106,107]]
        self.sorted_bass_exp = sorted(x, key=lambda tup: tup[0])
        y = [(note.start,note.end,note.pitch) for note in self.treble_exp.notes if note.pitch not in [20,21,106,107]]
        self.sorted_treble_exp = sorted(y, key=lambda tup: tup[0])

        # then merge expression according to different combinations
        self.mergeVelocity(option = "left_hand")
        self.mergeVelocity(option = "right_hand")

        # Adding Velocity
        self.addVelocity(left=True)
        self.addVelocity(left=False)

        # Adding Pedal
        if self.readPedal is True:
            self.addPedal()


    def writeOut(self):
        # Timbre and output
        outMidi = pretty_midi.PrettyMIDI()
        leftHand = pretty_midi.Instrument(program=pretty_midi.instrument_name_to_program('Acoustic Grand Piano'))
        rightHand = pretty_midi.Instrument(program=pretty_midi.instrument_name_to_program('Acoustic Grand Piano'))

        outMidi.instruments.append(self.left)
        outMidi.instruments.append(self.right)

        # Write out the MIDI data
        outMidi.write(self.out_path)
        print 'write to : ', self.out_path


    def addPedal(self):
        # adding soft pedal
        softOn = 0.0
        softOff = 0.0

        # add soft pedal
        for note in self.bass_exp.notes:
            if note.pitch == 21:
                if softOff != 0.0:
                    print 'Warning! previous soft pedal not off before ' + str(note.start)
                softOff = 0.0
                softOn = note.start
            elif note.pitch == 20:
                #if softOn == 0.0:
                #    print "Warning no soft pedal on detected before " + str(note.start)
                softOff = note.end
                #print 'soft pedal on from: ' + str(softOn) + ' to: ' + str(softOff)
                softControlOn = pretty_midi.ControlChange(67,70,softOn)
                softControlOff = pretty_midi.ControlChange(67,0,softOff)
                self.left.control_changes.append(softControlOn)
                self.left.control_changes.append(softControlOff)
                self.right.control_changes.append(softControlOn)
                self.right.control_changes.append(softControlOff)

                softOn = 0.0
                softOff = 0.0


        # add sustainPedal
        pedalOn = 0.0
        pedalOff = 0.0

        for note in self.treble_exp.notes:
            if note.pitch == 106:
                #if pedalOff != 0.0:
                #    print 'Warning! previous sustain pedal not off before ' + str(note.start)
                pedalOff = 0.0
                pedalOn = note.start
            elif note.pitch == 107:
                #if pedalOn == 0.0:
                #    print "Warning no sustain pedal on detected before " + str(note.start)
                pedalOff = note.end
                sustainControlOn = pretty_midi.ControlChange(64,70,pedalOn)
                sustainControlOff = pretty_midi.ControlChange(64,0,pedalOff)
                self.left.control_changes.append(sustainControlOn)
                self.left.control_changes.append(sustainControlOff)
                self.right.control_changes.append(sustainControlOn)
                self.right.control_changes.append(sustainControlOff)

                pedalOn = 0.0
                pedalOff = 0.0


    def addVelocity(self,left=False):
        if left is True:
            mynotes = self.left.notes
            myexp = self.exp_left
        else:
            mynotes = self.right.notes
            myexp = self.exp_right

        for noteI in range(len(mynotes)):
            # find closest velocity of the begin
            notebegin = int(mynotes[noteI].start  * 1000)
            noteend = int(mynotes[noteI].end  * 1000)
            i = min(notebegin,len(myexp)-1)
            v = myexp[i]
            if v == 0:
                v = self.getPreviousNonzero(myexp,i)
            # if still equals 0, map it to 60
            vv = min(v,127)
            vv = max(0,vv)
            mynotes[noteI].velocity = int(vv)
            if left is True:
                mynotes[noteI].velocity = max(int(vv) + self.left_adjust, 0) # in case less than 0
                self.left.notes = mynotes
            else:
                self.right.notes = mynotes


    def getPreviousNonzero(self,myArray,i):
        """
        Local function to get a nonzero previous value
        """
        while i >= 0:
            if myArray[i] > 0:
                return myArray[i]
            i = i-1;
        if i < 0:
            #print '[Warning, did not find a velocity before this note, map to vel_mid (default 66)]'
            return self.Welte_MF


    # def getCresendoLinear(self,startV,targetV,realL,speed,Ctype):
    #     """
    #     Local Function to generate an array of fast (2) or normal (1) crescendo (type 1) or decresendo (type -1)
    #     Linear version
    #     """
    #     arr = np.arange(realL) * 0.0
    #     L = 700 * 3
    #     # fast decrescendo
    #     if Ctype == -1 and speed == "fastCrescendo":
    #         L = 150
    #     # slow decrescendo
    #     elif Ctype == -1 and speed == "slowCrescendo":
    #         L = 2380
    #     # fast crescendo
    #     elif Ctype == 1 and speed == "fastCrescendo":
    #         L = 700
    #     # slow crescendo
    #     elif Ctype == 1 and speed == "slowCrescendo":
    #         L = 2380

    #     # constructing the linear curve
    #     step = (targetV-startV)*1./L
    #     y = np.arange(L) * 0.0
    #     y[0] = startV
    #     for i in range(1,L):
    #         y[i] = y[i-1] + step

    #     if realL < L:
    #         arr = y[:realL]
    #     else:
    #         arr[:L] = y
    #         arr[L:] = arr[L-1]
    #     return arr

    def mergeVelocity(self,option = "left_hand"):
        """
        As of 2018-04-06, some modification (F: fast crescendo -- length of perforation)
        (F+slow crescendo: fastest crescendo)
        Using Peter's velocity mapping: min30 MF60 Loud70 Max85
        dynamic range default 1.2, making Welte_MF: 80, Max (F): 96, and Min (P) 66
        left_hand: 12 less than right hand

        According to the following expression code of Red Welte
        Midi track 3:
           14: (1)Bass MF off
           15: (2)Bass MF on
           16: (3)Bass Crescendo off (Slow Crescendo)
           17: (4)Bass Crescendo on
           18: (5)Bass Forzando off  (Fast Crescendo)
           19: (6)Bass Forzando on
           20: (7)Soft-pedal off
           21: (8)Soft-pedal on
           22: Motor off
           23: Motor on

        Midi track 4:
           104: Rewind
           105: Electric cutoff
           106: (8)Sustain pedal on
           107: (7)Sustain pedal off
           108: (6)Treble Forzando on   (Fast Crescendo)
           109: (5)Treble Forzando off
           110: (4)Treble Crescendo on  (Slow Crescendo)
           111: (3)Treble Crescendo off
           112: (2)Treble MF on
           113: (1)Treble MF off

        """

        # Initial Setup of the expression curve
        if option == "left_hand":
            my_exp = self.bass_exp.notes
            exp_length = int(self.left.notes[-1].end*1000)
        else:
            my_exp = self.treble_exp.notes
            exp_length = int(self.right.notes[-1].end*1000)

        #ex = np.arange(exp_length)*self.minV*1.0  # an array of continuous velocity (set up as min)
        ex = np.ones(exp_length)*self.minV*1.0     # 0606 bug fix, initialize as minV
        isMF    = np.arange(exp_length) * 0        # setting up the upper/lower bound
        isSlowC = np.arange(exp_length) * 0        # slow crescendo on?
        isFastC = np.arange(exp_length) * 0        # fast crescendo on?
        isFastD = np.arange(exp_length) * 0        # fast decrescendo on?

        # Lock and Cancel
        Valve_MF_On = False
        Valve_SlowC_On = False

        Valve_MF_StartTime = 0         # False for off
        Valve_SlowC_StartTime = 0

        # First pass: For each time section calculate the current boolean state of each expression
        for currE in my_exp:
            exp_no = currE.pitch # expression number
            st = int(currE.start * 1000)
            et = int(currE.end * 1000)

            if exp_no == 14 or exp_no == 113:       # MF off
                if Valve_MF_On:
                    isMF[Valve_MF_StartTime:st] = 1 # record MF Valve information for previous
                Valve_MF_On = False


            elif exp_no == 15 or exp_no ==112:      # MF on, just update the start Time
                if not Valve_MF_On:                 # if previous has an on, ignore
                    Valve_MF_On = True
                    Valve_MF_StartTime = st

            elif exp_no == 16 or exp_no ==111:      # Crescendo Off (slow)
                if Valve_SlowC_On:
                    isSlowC[Valve_SlowC_StartTime:st] = 1     # record Cresc Valve information for previous
                Valve_SlowC_On = False
                #isSlowC[st:] = 0

            elif exp_no == 17 or exp_no ==110:      # Crescendo on (slow)
                if not Valve_SlowC_On:              # if previous has an on, ignore
                    Valve_SlowC_On = True
                    Valve_SlowC_StartTime = st

            # Fast Crescendo/Decrescendo is a direct operation (length of perforation matters)
            elif exp_no == 18 or exp_no ==109:      # Forzando off -- Fast Decrescendo
                isFastD[st:et] = 1
            elif exp_no == 19 or exp_no ==108:      # Forzando on -- fast Crescendo
                isFastC[st:et] = 1

        #TODO: deal with the last case (if crescendo OFF is missing)

        # Second pass, update the current velocity according to the previous one
        for i in range(1,exp_length):
            if isSlowC[i] == 0 and isFastC[i] == 0 and isFastD[i] == 0:
                amount = -self.slow_step   # slow decrescendo is always on
            # if both slow crescendo and fast crescendo
            #elif isSlowC[i] == 1 and isFastC[i] == 1:
            #    amount = self.slow_step + self.fastC_step
            else:
                amount = isSlowC[i] * self.slow_step + isFastC[i] * self.fastC_step + isFastD[i] * self.fastD_step
            newV = ex[i-1] + amount
            ex[i] = newV
            # safety check for regulator
            # if isMF[i]:
            #     # if it's increasing
            #     if amount > 0:
            #         if ex[i-1] <= self.Welte_MF:
            #             ex[i] = min(newV, self.Welte_MF)    # make sure not exceed
            #     # if it's decreasing
            #     else:
            #         if ex[i-1] >= self.Welte_MF:
            #             ex[i] = max(newV, self.Welte_MF)    # make sure not lower
            if isMF[i]:
                # if it's increasing
                if ex[i-1] > self.Welte_MF and amount > 0:
                    pass
                elif ex[i-1] > self.Welte_MF and amount < 0:
                    ex[i] = max(self.Welte_MF,ex[i])
                    #print 'clipping to not less than 60'
                elif ex[i-1] < self.Welte_MF and amount < 0:
                    pass
                elif ex[i-1] < self.Welte_MF and amount > 0:
                    ex[i] = min(self.Welte_MF, ex[i])
                    #print 'clipping to not greater than 60'
            else:
                # new: adding loud
                # slow crescendo will only reach LOUD
                if isSlowC[i] and isFastC[i] == 0:
                    ex[i] = min(ex[i],self.LOUD)
                #else:
            ex[i] = max(self.minV, ex[i])
            # regulating max
            ex[i] = min(self.maxV, ex[i])

        if option == "left_hand":
            self.exp_left = ex
        else:
            self.exp_right = ex


