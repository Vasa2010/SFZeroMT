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

#ifndef RIFF_H_INCLUDED
#define RIFF_H_INCLUDED

#include "SF2WinTypes.h"

namespace sfzero
{
    
    struct RIFFChunk
    {
        enum Type
        {
            RIFF,
            LIST,
            Custom
        };
        
        fourcc id;
        dword size;
        Type type;
        SamplePosition start;
        
        void readFrom(juce::InputStream *file);
        void seek(juce::InputStream *file);
        void seekAfter(juce::InputStream *file);
        
        SamplePosition end() { return (start + size); }
        juce::String readString(juce::InputStream *file);
    };
}

#endif // RIFF_H_INCLUDED
