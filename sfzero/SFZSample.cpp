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

#include "SFZSample.h"
#include "SFZDebug.h"

using namespace juce;
using namespace sfzero;

bool Sample::load (AudioFormatManager *formatManager)
{
    ScopedPointer<AudioFormatReader> reader (formatManager->createReaderFor(file_));
    
    if (reader == nullptr)
        return false;
    
    DBG ("Loading Sample " << file_.getFullPathName());
    
    sampleRate_   = reader->sampleRate;
    sampleLength_ = reader->lengthInSamples;
    // Read some extra samples, which will be filled with zeros, so interpolation
    // can be done without having to check for the edge all the time.
    jassert(sampleLength_ < std::numeric_limits<int>::max());
    
    buffer_ = new AudioSampleBuffer (reader->numChannels, static_cast<int>(sampleLength_ + 4));
    reader->read (buffer_, 0, static_cast<int>(sampleLength_ + 4), 0, true, true);
    
    StringPairArray *metadata = &reader->metadataValues;
    int numLoops = metadata->getValue("NumSampleLoops", "0").getIntValue();
    if (numLoops > 0)
    {
        loopStart_ = metadata->getValue("Loop0Start", "0").getLargeIntValue();
        loopEnd_   = metadata->getValue("Loop0End", "0").getLargeIntValue();
    }
    return true;
}

Sample::~Sample()
{
}

String Sample::getShortName()
{
    return (file_.getFileName());
}

void Sample::setBuffer (AudioSampleBuffer *newBuffer)
{
    buffer_ = newBuffer;
    sampleLength_ = buffer_->getNumSamples();
}

AudioSampleBuffer *Sample::detachBuffer()
{
    AudioSampleBuffer *result = buffer_;
    buffer_ = nullptr;
    return result;
}

String Sample::dump()
{
    return file_.getFullPathName() + "\n";
}

#ifdef JUCE_DEBUG
void Sample::checkIfZeroed (const char *where)
{
    if (buffer_ == nullptr)
    {
        dbgprintf("Sample::checkIfZeroed(%s): no buffer!", where);
        return;
    }
    
    int samplesLeft = buffer_->getNumSamples();
    int64 nonzero = 0, zero = 0;
    const float *p = buffer_->getReadPointer(0);
    for (; samplesLeft > 0; --samplesLeft)
    {
        if (*p++ == 0.0)
        {
            zero += 1;
        }
        else
        {
            nonzero += 1;
        }
    }
    if (nonzero > 0)
    {
        dbgprintf("Buffer not zeroed at %s (%lu vs. %lu).", where, nonzero, zero);
    }
    else
    {
        dbgprintf("Buffer zeroed at %s!  (%lu zeros)", where, zero);
    }
}

#endif // JUCE_DEBUG
