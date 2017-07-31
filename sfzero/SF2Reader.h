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

#ifndef SF2READER_H_INCLUDED
#define SF2READER_H_INCLUDED

#include "SF2.h"

/** 
 The spec says "initialAttenuation" is in centibels. But everyone seems to treat it as millibels.
 Since this was different with individual SoundFonts, maybe let's use an average:
 */
#define SF2_INITIAL_ATTENUATION_TO_DB -0.05f

namespace sfzero
{
    
    class  SF2Sound;
    class  Sample;
    struct Region;
    
    class SF2Reader
    {
    public:
        SF2Reader(SF2Sound *sound, const juce::File &file);
        virtual ~SF2Reader();
        
        void read();
        
        /** Reads shared sample data of all samples */
        juce::AudioSampleBuffer *readSampleData (double *progressVar = nullptr, juce::Thread *thread = nullptr);
        
    private:
        SF2Sound *sound_;
        juce::ScopedPointer<juce::FileInputStream> file_;
        
        void addGeneratorToRegion (word genOper, SF2::genAmountType *amount, Region *region);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SF2Reader)
    };
}

#endif // SF2READER_H_INCLUDED
