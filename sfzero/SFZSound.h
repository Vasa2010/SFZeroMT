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


#ifndef SFZSOUND_H_INCLUDED
#define SFZSOUND_H_INCLUDED

#include "SFZRegion.h"
#include "SFZExtensions.h"
#include "SFZSharedResources.h"

namespace sfzero
{
    
    class Sample;
    class SFZReader;
    
    typedef juce::OwnedArray<ProgramSelection> ProgramList;
    
    class Sound : public juce::SynthesiserSound
    {
    public:
        explicit Sound (const juce::File &file, int channel);
        virtual ~Sound ();
        
        SharedResourcesSFZ* sharedSamples();
        
        typedef juce::ReferenceCountedObjectPtr<Sound> Ptr;
        
        juce::File &getFile() { return file_; }
        
        bool appliesToNote (int midiNoteNumber) override;
        bool appliesToChannel (int midiChannel) override;
        
        // Region access
        int getNumRegions();
        juce::Array<Region *> &getRegions() { return regions_; }
        Region *getRegionFor (int note, int velocity, Region::Trigger trigger = Region::attack);
        Region *regionAt (int index);
        
        // Loading & building the sound
        virtual void loadRegions ();
        virtual void loadSamples (juce::AudioFormatManager *formatManager,
                                  double *progressVar = nullptr,
                                  juce::Thread *thread = nullptr);
        void addRegion (Region *region); // Takes ownership of the region.
        Sample *addSample (juce::String path, juce::String defaultPath = "");
        
        // Logging & info
        void addError (const juce::String &message);
        void addUnsupportedOpcode (const juce::String &opcode);
        const juce::StringArray &getErrors() { return errors_; }
        const juce::StringArray &getWarnings() { return warnings_; }
        juce::String dump();
        
    protected:
        friend class Synth;
        // program selection must run under lock protection from Synth
        virtual int               getProgramCount(int bank);
        virtual juce::String      getProgramName (const ProgramSelection& selection);
        virtual ProgramSelection& getProgramSelection ();
        virtual void              setProgramSelection (const ProgramSelection& selection);
        virtual ProgramList*      getProgramList();
        
        int channel_;
        ProgramSelection selection_;
        juce::File file_;
        
    private:
        juce::Array<Region *> regions_;
        juce::StringArray errors_;
        juce::StringArray warnings_;
        juce::HashMap<juce::String, juce::String> unsupportedOpcodes_;
        SharedResourcesSFZ::Ptr sfzSamples_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sound)
    };
    
}

#endif // SFZSOUND_H_INCLUDED
