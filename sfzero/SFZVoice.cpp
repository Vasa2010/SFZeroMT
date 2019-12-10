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

#include "SFZDebug.h"
#include "SFZRegion.h"
#include "SFZSample.h"
#include "SFZSound.h"
#include "SFZVoice.h"

using namespace juce;
using namespace sfzero;

Voice::Voice() :
    region(nullptr),
    trigger(0),
    curMidiNote(0),
    curPitchWheel(0),
    noteGainL(0),
    noteGainR(0),
    pitchRatio(1),
    sourceSamplePosition(0),
    sampleStart(0),
    sampleEnd(0),
    loopStart(0),
    loopEnd(0),
    loopCounter(0),
    curVelocity(0)
{
    ampeg.setExponentialDecay(true);
}

Voice::~Voice()
{
}

bool Voice::canPlaySound (SynthesiserSound *sound)
{
    return dynamic_cast<Sound *>(sound) != nullptr;
}

void Voice::startNote(int midiNoteNumber,
                      float floatVelocity,
                      SynthesiserSound *soundIn,
                      int currentPitchWheelPosition)
{
    Sound *sound = dynamic_cast<Sound *>(soundIn);
    
    if (sound == nullptr)
    {
        killNote();
        return;
    }
    
    int velocity = static_cast<int>(floatVelocity * 127.0);
    curVelocity = velocity;
    if (region == nullptr)
    {
        region = sound->getRegionFor (midiNoteNumber, velocity);
    }
    if ((region == nullptr) || (region->sample == nullptr) || (region->sample->getBuffer() == nullptr))
    {
        killNote();
        return;
    }
    if (region->negative_end)
    {
        killNote();
        return;
    }
    
    // Pitch.
    curMidiNote = midiNoteNumber;
    curPitchWheel = currentPitchWheelPosition;
    calcPitchRatio();
    
    // Gain.
    double noteGainDB = region->volume;
    
    // Thanks to <http:://www.drealm.info/sfz/plj-sfz.xhtml> for explaining the
    // velocity curve in a way that I could understand, although they mean
    // "log10" when they say "log".
    double velocityGainDB = -20.0 * log10((127.0 * 127.0) / (velocity * velocity));
    velocityGainDB *= region->amp_veltrack / 100.0;
    noteGainDB += velocityGainDB;
    noteGainL = noteGainR = static_cast<float>(Decibels::decibelsToGain(noteGainDB));
    
    // The SFZ spec is silent about the pan curve, but a 3dB pan law seems
    // common.  This sqrt() curve matches what Dimension LE does; Alchemy Free
    // seems closer to sin(adjustedPan * pi/2).
    double adjustedPan = (region->pan + 100.0) / 200.0;
    noteGainL *= static_cast<float>(sqrt(1.0 - adjustedPan));
    noteGainR *= static_cast<float>(sqrt(adjustedPan));
    ampeg.startNote(&region->ampeg, floatVelocity, getSampleRate(), &region->ampeg_veltrack);
    
    // Offset/end.
    sourceSamplePosition = static_cast<double>(region->offset);
    sampleStart = region->offset;
    sampleEnd = region->sample->getSampleLength();
    if ((region->end > 0) && (region->end < sampleEnd))
    {
        sampleEnd = region->end + 1;
    }
    
    // Loop.
    loopStart = loopEnd = 0;
    Region::LoopMode loopMode = region->loop_mode;
    if (loopMode == Region::sample_loop)
    {
        if (region->sample->getLoopStart() < region->sample->getLoopEnd())
        {
            loopMode = Region::loop_continuous;
        }
        else
        {
            loopMode = Region::no_loop;
        }
    }
    if ((loopMode != Region::no_loop) && (loopMode != Region::one_shot))
    {
        if (region->loop_start < region->loop_end)
        {
            loopStart = region->loop_start;
            loopEnd = region->loop_end;
        }
        else
        {
            loopStart = region->sample->getLoopStart();
            loopEnd = region->sample->getLoopEnd();
        }
    }
    loopCounter = 0;
}

void Voice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (!allowTailOff || (region == nullptr))
    {
        killNote();
        return;
    }
    
    if (region->loop_mode != Region::one_shot)
    {
        ampeg.noteOff();
    }
    if (region->loop_mode == Region::loop_sustain)
    {
        // Continue playing, but stop looping.
        loopEnd = loopStart;
    }
}

void Voice::stopNoteForGroup()
{
    if (region->off_mode == Region::fast)
    {
        ampeg.fastRelease();
    }
    else
    {
        ampeg.noteOff();
    }
}

void Voice::stopNoteQuick()
{
    ampeg.fastRelease();
}

void Voice::pitchWheelMoved (int newValue)
{
    if (region == nullptr)
        return;
    curPitchWheel = newValue;
    calcPitchRatio();
}

void Voice::controllerMoved (int controllerNumber, int newValue)
{
    switch (controllerNumber)
    {
        case 121:
            // Reset All Controllers
            curPitchWheel = 0.0f;
            break;
            
        default:
            break;
    }
}

void Voice::renderNextBlock (AudioSampleBuffer &outputBuffer, int startSample, int numSamples)
{
    if (region == nullptr)
    {
        return;
    }
    // Simplified the code below for readability,
    // Speculating on register-optimization does probably not make much sense.
    // Fixed issues with very narrow loops.
    
    AudioSampleBuffer *buffer = region->sample->getBuffer();
    int bufferSize = buffer->getNumSamples();
    
    const float *inL = buffer->getReadPointer(0, 0);
    const float *inR = buffer->getNumChannels() > 1 ? buffer->getReadPointer(1, 0) : nullptr;
    
    float  *outL = outputBuffer.getWritePointer(0, startSample);
    float  *outR = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1, startSample) : nullptr;
    
    float  ampegGain = ampeg.getLevel();
    float  ampegSlope = ampeg.getSlope();
    int    samplesUntilNextAmpSegment = ampeg.getSamplesUntilNextSegment();
    bool   ampSegmentIsExponential = ampeg.getSegmentIsExponential();
    bool   looping = (loopStart < loopEnd);
    
    while (--numSamples >= 0)
    {
        // Simple linear interpolation between neighboring samples @ pos1, pos2
        const SamplePosition pos1 = floor(sourceSamplePosition);
        const float alpha = sourceSamplePosition - (double)pos1;
        const float alphaInv = 1.0f - alpha;
        SamplePosition pos2 = pos1 + 1;
        
        if (looping && (pos2 > loopEnd))
            pos2 = loopStart;
        
        if (pos2 > bufferSize)
            pos2 = bufferSize;
        
        jassert(pos1 >= 0 && pos1 < bufferSize && pos2 >= 0 && pos2 < bufferSize);
        
        float l = (inL[pos1] * alphaInv + inL[pos2] * alpha);
        float r = inR ? (inR[pos1] * alphaInv + inR[pos2] * alpha) : l;
        
        // Shouldn't we dither here?
        l *= (noteGainL * ampegGain);
        r *= (noteGainR * ampegGain);
        
        if (outR)
        {
            *outL++ += l;
            *outR++ += r;
        }
        else
        {
            *outL++ += (l + r) * 0.5f;
        }
        
        // Advance to next sample
        sourceSamplePosition += pitchRatio;
        // Wrap around loop, if necessary
        if (looping && (sourceSamplePosition >= loopEnd))
        {
            sourceSamplePosition = loopStart + (sourceSamplePosition - loopEnd);
            loopCounter++;
        }
        
        // Update EG
        if (ampSegmentIsExponential)
        {
            ampegGain *= ampegSlope;
        }
        else
        {
            ampegGain += ampegSlope;
        }
        if (--samplesUntilNextAmpSegment < 0)
        {
            ampeg.setLevel(ampegGain);
            ampeg.nextSegment();
            ampegGain = ampeg.getLevel();
            ampegSlope = ampeg.getSlope();
            samplesUntilNextAmpSegment = ampeg.getSamplesUntilNextSegment();
            ampSegmentIsExponential = ampeg.getSegmentIsExponential();
        }
        
        if ((sourceSamplePosition >= sampleEnd) || ampeg.isDone())
        {
            killNote();
            break;
        }
    }
    
    ampeg.setLevel(ampegGain);
    ampeg.setSamplesUntilNextSegment(samplesUntilNextAmpSegment);
}

bool Voice::isPlayingNoteDown()
{
    return region && region->trigger != Region::release;
}

bool Voice::isPlayingOneShot()
{
    return region && region->loop_mode == Region::one_shot;
}

int Voice::getGroup()
{
    return region ? region->group : 0;
}

uint64 Voice::getOffBy()
{
    return region ? region->off_by : 0;
}

void Voice::setRegion(Sound* currentSoud, Region *nextRegion)
{
    sound = currentSoud;
    region = nextRegion;
}

String Voice::infoString()
{
    const char *egSegmentNames[] = { "delay", "attack", "hold", "decay", "sustain", "release", "done" };
    
    const static int numEGSegments(sizeof(egSegmentNames) / sizeof(egSegmentNames[0]));
    
    const char *egSegmentName = "-Invalid-";
    int egSegmentIndex = ampeg.segmentIndex();
    if ((egSegmentIndex >= 0) && (egSegmentIndex < numEGSegments))
    {
        egSegmentName = egSegmentNames[egSegmentIndex];
    }

#if JUCE_DEBUG
    SF2Sound* sf2sound = dynamic_cast<SF2Sound*>(sound);
    String* namePtr = (sf2sound == nullptr)
        ? nullptr
        : sf2sound->sharedSamples()->sampleNameAt(region->offset);
#else
    String* namePtr = nullptr;
#endif
    String info;
    info << "note: " << curMidiNote
        << ", vel: " << curVelocity
//      << ", pan: " << region->pan
        << ", sample: " << (namePtr == nullptr ? "?" : *namePtr)
        << ", rate: " << region->sample->getSampleRate()
        << ", root: " << region->pitch_keycenter
        << ", trp: " << region->transpose
        << "/" << region->tune
        << ", trk: " << region->pitch_keytrack
//      << ", eg: " << egSegmentName
        << ", loops: " << loopCounter;
    return info;
}

void Voice::calcPitchRatio()
{
    double pitch = curMidiNote;
    
    pitch += region->transpose;
    pitch += region->tune / 100.0;
    
    if (region->pitch_keytrack != 100)
        pitch = region->pitch_keycenter + (pitch - region->pitch_keycenter) * (region->pitch_keytrack / 100.0);
    
    if (curPitchWheel != 8192)
    {
        double wheel = ((2.0 * curPitchWheel / 16383.0) - 1.0);
        if (wheel > 0)
            pitch += wheel * region->bend_up / 100.0;
        else
            pitch += wheel * region->bend_down / -100.0;
    }
    //double targetFreq = fractionalMidiNoteInHz(adjustedPitch);
    //double naturalFreq = MidiMessage::getMidiNoteInHertz(region->pitch_keycenter);
    //pitchRatio = (targetFreq * region->sample->getSampleRate()) / (naturalFreq * getSampleRate());
    
    // Code taken from from juce::SamplerVoice
    pitchRatio = pow (2.0, (pitch - (double)region->pitch_keycenter) / 12.0) * region->sample->getSampleRate() / getSampleRate();
}

void Voice::killNote()
{
    region = nullptr;
    clearCurrentNote();
}

