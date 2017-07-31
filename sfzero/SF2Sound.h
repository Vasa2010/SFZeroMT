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

#ifndef SF2SOUND_H_INCLUDED
#define SF2SOUND_H_INCLUDED

#include "SFZSound.h"
#include "SFZExtensions.h"

namespace sfzero
{
    class SharedResourcesSF2;
    
    class SF2Sound : public Sound
    {
    public:
        explicit SF2Sound (const juce::File &fileIn, int channel);
        virtual ~SF2Sound ();
        
        SharedResourcesSF2* sharedSamples();
        
        void loadRegions() override;
        void loadSamples(juce::AudioFormatManager *formatManager,
                         double *progressVar = nullptr,
                         juce::Thread *thread = nullptr) override;
        
        
        void addPreset(Preset *preset);
        
        // Presets, channel and program selection:
        // Pass -1 for bank to request all banks
        int               getProgramCount(int bank) override;
        juce::String      getProgramName (const ProgramSelection& selection) override;
        ProgramSelection& getProgramSelection () override;
        void              setProgramSelection (const ProgramSelection& selection) override;
        ProgramList*      getProgramList () override;
        
        Sample* sampleFor (double sampleRate);
        
    private:
        
        bool hasBank (int bank);
        juce::HashMap<int, Preset*> presets_;
        SharedResourcesSF2::Ptr sf2Samples_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SF2Sound)
    };
}

#endif // SF2SOUND_H_INCLUDED
