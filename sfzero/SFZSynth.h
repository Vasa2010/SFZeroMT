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

#ifndef SFZSYNTH_H_INCLUDED
#define SFZSYNTH_H_INCLUDED

#include "SFZCommon.h"
#include "SFZExtensions.h"

namespace sfzero
{
    class Synth :
        public juce::Synthesiser,
        public juce::ChangeBroadcaster
    {
    public:
        
        // Parameters exposed to the GUI and MIDI control
        typedef enum {
            kParam_Volume = 0,
            kParam_Pan,
            kParam_Send,
            NumParameters
        } Parameters;
        
        Synth (int channel);
        virtual ~Synth();
        
        // Safely swap sound under lock
        void swapSound (const juce::SynthesiserSound::Ptr &newSound);
        
        // Safely select and query presets under lock
        virtual int               getProgramCount(int bank);
        virtual juce::String      getProgramName (const ProgramSelection& selection);
        virtual ProgramSelection& getProgramSelection ();
        virtual void              setProgramSelection (const ProgramSelection& selection);
        virtual ProgramList*      getProgramList();
        
        void noteOn  (int midiChannel, int midiNoteNumber, float velocity) override;
        void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override;
        
        // Filters all messages not on my channel */
        void handleMidiEvent      (const juce::MidiMessage& m) override;
        // Handles bank & program selection and volume, pan, reverb, etc
        void handleController     (int midiChannel, int controllerNumber, int controllerValue) override;
        void handleProgramChange  (int midiChannel, int programNumber) override;
        // Implement master volume & pan here:
        void renderVoices (juce::AudioSampleBuffer &outputAudio, int startSample, int numSamples) override;
        
        juce::String voiceInfoString();
        int numVoicesUsed();        
        
        // Control volume, pan & reverb send, range 0...1 (see: Synth::Parameters)
        float getParameter (int index);
        void  setParameter (int index, float newValue);
        
        std::unique_ptr<juce::XmlElement> getStateXML ();
        bool setStateXML (const juce::XmlElement* xml);
        
        /** Allow my ChangeListener to distinguish between program selection or other parameter changes */
        bool hasProgramSelectionChanged (bool reset = true);
        
        bool usesEffectsUnit();
        
        /** Return the only sound (soundbank actually), typecast to sfzero::Sound */
        Sound* getSound ();
        
    private:
        
        int channel_;
        int noteVelocities_[128];
        ProgramSelection selectionCache_;
        int selectedBank_MSB_;
        juce::Atomic<int>   selectionChanged;
        juce::Atomic<int>   sendLevelCC_;
        juce::Atomic<float> sendLevel_;
        juce::Atomic<int>   masterVolumeCC_;
        juce::Atomic<float> masterVolume_;
        juce::Atomic<int>   masterPanCC_;
        juce::Atomic<float> masterPanL_, masterPanR_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Synth)
    };
}

#endif // SFZSYNTH_H_INCLUDED
