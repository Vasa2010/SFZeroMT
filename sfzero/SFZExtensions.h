/***********************************************************************
 *  SFZeroMT Multi-Timbral Juce Module
 *
 *  Original SFZero Copyright (C) 2012 Steve Folta
 *      https://github.com/stevefolta/SFZero
 *  Converted to Juce module Copyright (C) 2016 Leo Olivers
 *      https://github.com/altalogix/SFZero
 *  Extended for multi-timbral operation Copyright (C) 2017 Cognitone
 *      https://github.com/cognitone/SFZeroMT
 *
 *  Licensed under MIT License - Please read regard LICENSE document
 ***********************************************************************/

#ifndef SFZEXTENSIONS_H_INCLUDED
#define SFZEXTENSIONS_H_INCLUDED

#include "SFZCommon.h"

namespace sfzero {
    
    /**
     Implements a linear program index split into banks per 128 programs
     suitable for use with MIDI messages and for sorting presets.
     */
    class ProgramSelection
    {
    public:
        virtual ~ProgramSelection() {}
        ProgramSelection () : bank(0), program(0) {}
        ProgramSelection (int inBank, int inProgram) : bank(inBank), program(inProgram) {}
        ProgramSelection (int inBank, int inProgram, const juce::String& inName) : bank(inBank), program(inProgram), name(inName) {}
        ProgramSelection (int index) : bank((int)(index / 128)), program(index % 128) {}
        
        // For sorting & HashMap
        int index() const { return (bank * 128) + program; }
        bool equals (const ProgramSelection& p) const { return bank == p.bank && program == p.program; }
        
        int bankMSB() { return bank / 128; }
        int bankLSB() { return bank % 128; }
        
        int bank;
        int program;
        juce::String name; // optional, unless used with Preset or for GUI
    };
    
    /** Sorting for menus and linear lists */
    class ProgramSelectionComparator
    {
    public:
        static int compareElements (const sfzero::ProgramSelection *first,
                                    const sfzero::ProgramSelection *second)
        {
            return first->index() - second->index();
        }
    };
    
    /** Preset is used by SF2Sound only */
    class Preset
    {
    public:
        virtual ~Preset() {}
        Preset (const juce::String nameIn, int bankIn, int presetIn) : selection(bankIn, presetIn, nameIn) {}
        
        int index() const { return selection.index(); }
        juce::String& getName () { return selection.name; }
        ProgramSelection& getSelection () { return selection; }
        
        void addRegion (Region *region) { regions.add(region); }
        
        ProgramSelection selection;
        juce::OwnedArray<Region> regions;
        
        JUCE_LEAK_DETECTOR (Preset)
    };
    
    
    /** Conversions from linear knobs/faders (0...1) to audio multipliers (gain) and vice versa  */
    
    /** Maps a linear knob (0..1) to u-Law pan multipliers for channels L,R. Center value is 0.5. */
    static void convertFaderToPan (const float fader, float& scaleLeft, float& scaleRight)
    {
        float pan = juce::jlimit (0.0f, 1.0f, fader);
        scaleLeft = sqrt(1.0f - pan);
        scaleRight = sqrt(pan);
    }
    
    /** These parameters resemble a default mixing console dB scaling */
#define FaderFactorA6dB   0.002
#define FaderFactorB6dB   6.908
    
#define FaderFactorA0dB   0.0012
#define FaderFactorB0dB   6.726
    
    /** Linear fader position that returns 1.0 for +6dB mapping */
#define FaderPosUnity     0.811024
#define FaderINFPos       0.006
#define FaderSnapUnity    0.0066
    
    /** Maps linear fader/knob position (0..1) to gain multiplier 0.0 (-INF) ... ~2.0 (+6dB) */
    static double convertFaderToGain6dB (const double fader, double faderCutOff = FaderINFPos)
    {
        if (fader < faderCutOff)
            return 0.0;
        // Resembles an analog mixer fader
        double gain = FaderFactorA6dB * exp(FaderFactorB6dB * sqrt( juce::jlimit (0.0, 1.0, fader)));
        if (fabs(gain - 1.0) < FaderSnapUnity)
            return 1.0;
        return gain;
    }
    
    /** Maps a gain multiplier 0.0 (-INF) ... ~2.0 (+6dB) to linear fader/knob position (0..1) */
    static double convertGainToFader6dB (const double gain, double faderCutOff = FaderINFPos)
    {
        // Inverse function to the above
        double fader =
        juce::jlimit (0.0,
                      1.0,
                      pow(log(juce::jlimit(0.0, 2.0, gain) / FaderFactorA6dB) / FaderFactorB6dB,
                          2.0));
        if (fader < faderCutOff)
            return 0.0;
        return fader;
    }
    
    /** Maps linear fader/knob position (0..1) to gain multiplier 0.0 (-INF) ... 1.0 (0dB) */
    static double convertFaderToGain0dB (const double fader, double faderCutOff = FaderINFPos)
    {
        if (fader < faderCutOff)
            return 0.0;
        // Resembles an analog mixer fader
        double gain = FaderFactorA0dB * exp(FaderFactorB0dB * sqrt( juce::jlimit (0.0, 1.0, fader)));
        return juce::jlimit(0.0, 1.0, gain);
    }
    
    /** Maps a gain multiplier 0.0 (-INF) ... 1.0 (0dB) to linear fader/knob position (0..1) */
    static double convertGainToFader0dB (const double gain, double faderCutOff = FaderINFPos)
    {
        // Inverse function to the above
        double fader =
        juce::jlimit (0.0,
                      1.0,
                      pow(log(juce::jlimit(0.0, 1.0, gain) / FaderFactorA0dB) / FaderFactorB0dB,
                          2.0));
        if (fader < faderCutOff)
            return 0.0;
        return fader;
    }
    
}


#endif // SFZEXTENSIONS_H_INCLUDED
