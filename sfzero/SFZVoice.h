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

#ifndef SFZVOICE_H_INCLUDED
#define SFZVOICE_H_INCLUDED

#include "SFZEG.h"

namespace sfzero
{
    struct Region;
    
    class Voice : public juce::SynthesiserVoice
    {
    public:
        Voice();
        virtual ~Voice();
        
        bool canPlaySound (juce::SynthesiserSound *sound) override;
        
        void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
        void stopNote (float velocity, bool allowTailOff) override;
        void stopNoteForGroup();
        void stopNoteQuick();
        
        void pitchWheelMoved (int newValue) override;
        void controllerMoved (int controllerNumber, int newValue) override;
        
        void renderNextBlock (juce::AudioSampleBuffer &outputBuffer, int startSample, int numSamples) override;
        
        bool isPlayingNoteDown();
        bool isPlayingOneShot();
        
        int getGroup();
        juce::uint64 getOffBy();
        
        // Set the region to be used by the next startNote().
        void setRegion (Sound* sound, Region *nextRegion);
        
        juce::String infoString();
        
    private:
        void    calcPitchRatio();
        void    killNote();
        
        Sound*  sound;
        Region* region;
        
        int     trigger;
        int     curMidiNote, curPitchWheel;
        float   noteGainL, noteGainR;
        double  pitchRatio;
        double  sourceSamplePosition;
        EG      ampeg;
        long    sampleStart, sampleEnd;
        long    loopStart, loopEnd;
        
        // Info only.
        int     loopCounter;
        int     curVelocity;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Voice)
    };
}

#endif // SFZVOICE_H_INCLUDED
