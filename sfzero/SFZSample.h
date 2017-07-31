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

#ifndef SFZSAMPLE_H_INCLUDED
#define SFZSAMPLE_H_INCLUDED

#include "SFZCommon.h"

namespace sfzero
{
    
    class Sample
    {
    public:
        
        explicit Sample (const juce::File &fileIn) :
            file_(fileIn),
            buffer_(nullptr),
            sampleRate_(0),
            sampleLength_(0),
            loopStart_(0),
            loopEnd_(0)
        {}
        
        explicit Sample (double sampleRateIn) :
            buffer_(nullptr),
            sampleRate_(sampleRateIn),
            sampleLength_(0),
            loopStart_(0),
            loopEnd_(0)
        {}
        
        virtual ~Sample();
        
        bool load (juce::AudioFormatManager *formatManager);
        
        juce::File getFile() { return file_; }
        juce::String getShortName();
        double getSampleRate() { return sampleRate_; }
        
        juce::AudioSampleBuffer *getBuffer() { return buffer_; }
        void setBuffer (juce::AudioSampleBuffer *newBuffer);
        juce::AudioSampleBuffer *detachBuffer();
        
        juce::String dump();
        juce::uint64 getSampleLength() const { return sampleLength_; }
        juce::uint64 getLoopStart() const { return loopStart_; }
        juce::uint64 getLoopEnd() const { return loopEnd_; }
        
#ifdef JUCE_DEBUG
        void checkIfZeroed (const char *where);
#endif
        
        // Keep sample name for debugging of SF2 samples
        juce::String name;
        
    private:
        juce::File file_;
        // With SF2, all samples point to a single buffer
        juce::AudioSampleBuffer *buffer_;
        double sampleRate_;
        juce::uint64 sampleLength_, loopStart_, loopEnd_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sample)
    };
}

#endif // SFZSAMPLE_H_INCLUDED
