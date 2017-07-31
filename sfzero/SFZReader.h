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

#ifndef SFZREADER_H_INCLUDED
#define SFZREADER_H_INCLUDED

#include "SFZCommon.h"

namespace sfzero
{
    
    struct Region;
    class  Sound;
    
    class Reader
    {
    public:
        explicit Reader(Sound *sound);
        ~Reader();
        
        void read(const juce::File &file);
        void read(const char *text, unsigned int length);
        
    private:
        const char *handleLineEnd(const char *p);
        const char *readPathInto(juce::String *pathOut, const char *p, const char *end);
        int  keyValue(const juce::String &str);
        int  triggerValue(const juce::String &str);
        int  loopModeValue(const juce::String &str);
        void finishRegion(Region *region);
        void error(const juce::String &message);
        
        Sound *sound_;
        int line_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reader)
    };
    
    class StringSlice
    {
    public:
        StringSlice(const char *startIn, const char *endIn) : start_(startIn), end_(endIn) {}
        virtual ~StringSlice() {}
        
        unsigned int length() { return static_cast<int>(end_ - start_); }
        bool operator == (const char *other) { return (strncmp(start_, other, length()) == 0); }
        bool operator != (const char *other) { return (strncmp(start_, other, length()) != 0); }
        const char *getStart() const { return start_; }
        const char *getEnd() const { return end_; }
    private:
        const char *start_;
        const char *end_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSlice)
    };
}

#endif // SFZREADER_H_INCLUDED
