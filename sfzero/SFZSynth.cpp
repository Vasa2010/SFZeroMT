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

#include "SFZSynth.h"
#include "SFZSound.h"
#include "SFZVoice.h"

using namespace juce;
using namespace sfzero;


Synth::Synth (int channel) :
    Synthesiser(),
    channel_(channel),
    selectionCache_(0,0, ""),
    selectedBank_MSB_(0),
    sendLevelCC_(0),
    masterVolumeCC_(90),
    masterPanCC_(64)
{
    // This translates MIDI CC to linear multiplicators for rendering
    setParameter(kParam_Volume, masterVolumeCC_.get()/127.0);
    setParameter(kParam_Pan, masterPanCC_.get()/127.0);
    setParameter(kParam_Send, sendLevelCC_.get()/127.0);
    selectionChanged.set(0);
}

Synth::~Synth ()
{
    sounds.clear();
}


void Synth::swapSound (const SynthesiserSound::Ptr &newSound)
{
    ScopedLock locker (lock);
    
    allNotesOff(0, false);
    sounds.clear();
    sounds.add(newSound);
}

Sound* Synth::getSound ()
{
    SynthesiserSound::Ptr s = Synthesiser::getSound(0);
    return dynamic_cast<Sound*>(s.get());
}

void Synth::setProgramSelection (const ProgramSelection& selection)
{
    ScopedLock locker (lock);
    
    Sound* sound = getSound();
    if (sound)
    {
        if (!selection.equals (sound->getProgramSelection()))
        {
            allNotesOff(0, false);
            sound->setProgramSelection(selection);
        }
        selectionCache_ = sound->getProgramSelection();
        
    } else {
        selectionCache_ = selection;
        selectionCache_.name = String();
    }
    jassert (selectionCache_.index() == selection.index());
    selectionChanged.set(1);
    sendChangeMessage();
    
    DBG ("  Channel = " << channel_
         << " Bank = " << selectionCache_.bank
         << "  Program = " << selectionCache_.program
         << " " << selectionCache_.name);
}

ProgramSelection& Synth::getProgramSelection ()
{
    ScopedLock locker (lock);
    
    Sound* sound = getSound();
    if (sound)
        return sound->getProgramSelection();
    else
        return selectionCache_;
}

int Synth::getProgramCount (int bank)
{
    ScopedLock locker (lock);
    
    Sound* sound = getSound();
    if (sound)
        return sound->getProgramCount(bank);
    else
        return 1;
}

String Synth::getProgramName (const ProgramSelection& selection)
{
    ScopedLock locker (lock);
    
    Sound* sound = getSound();
    if (sound)
        return sound->getProgramName(selection);
    else
        return String();
}

ProgramList* Synth::getProgramList()
{
    ScopedLock locker (lock);
    
    Sound* sound = getSound();
    if (sound)
        return sound->getProgramList();
    else
        return new ProgramList();
}

float Synth::getParameter (int index)
{
    // value range is 0...1 (fader position)
    switch (index)
    {
        case kParam_Volume:
            return masterVolumeCC_.get()/127.0;
            break;
            
        case kParam_Pan:
            return masterPanCC_.get()/127.0;
            break;
            
        case kParam_Send:
            return sendLevelCC_.get()/127.0;
            break;
            
        default:
            break;
    }
    return 0;
}

void  Synth::setParameter (int index, float newValue)
{
    // value range is 0...1 (fader position)
    float value = jlimit (0.0f, 1.0f, newValue);
    
    switch (index)
    {
        case kParam_Volume:
			masterVolumeCC_.set(juce::roundToInt(value * 127));
            masterVolume_.set(convertFaderToGain6dB (value));
            //DBG("fader=" << value << " gain=" << masterVolume_.get());
            break;
            
        case kParam_Pan:
            float l, r;
            masterPanCC_.set(juce::roundToInt(value * 127));
            convertFaderToPan (value, l, r);
            masterPanL_.set(l);
            masterPanR_.set(r);
            break;
            
        case kParam_Send:
            sendLevelCC_.set(juce::roundToInt(value * 127));
            sendLevel_.set(convertFaderToGain0dB (value));
            break;
            
        default:
            break;
    }
    sendChangeMessage();
}

bool Synth::usesEffectsUnit()
{
    return sendLevelCC_.get() > 0;
}

void Synth::handleMidiEvent (const MidiMessage& m)
{
    // Ignore all messages not on my channel
    if (m.getChannel() == channel_)
        Synthesiser::handleMidiEvent(m);
}

void Synth::handleController (int midiChannel, int controllerNumber, int controllerValue)
{
    switch (controllerNumber)
    {
        case 0:
            // Bank selection requires MSB, LSB in that order!
            selectedBank_MSB_ = controllerValue;
            return;
            break;
            
        case 32:
            // Final LSB will determine and set the current bank number
            selectionCache_.bank = 128 * selectedBank_MSB_ + controllerValue;
            selectedBank_MSB_ = 0;
            return;
            break;
            
        case 7:
            // Volume
            setParameter(kParam_Volume, controllerValue / 127.0);
            return;
            break;
            
        case 10:
            // Pan
            setParameter(kParam_Pan, controllerValue / 127.0);
            return;
            break;
            
        case 91:
            // Reverb Send
            setParameter(kParam_Send, controllerValue / 127.0);
            return;
            break;
            
        case 121:
            // Reset All Controllers
            setParameter(kParam_Send, 0);
            setParameter(kParam_Pan, 0.5f);
            setParameter(kParam_Volume, 90.0f / 127.0f);
            // fall through to superclass, so voices get to reset ModWheel, etc
            break;
            
        default:
            break;
    }
    // Superclass handles pedals, passes CC on to each voice
    Synthesiser::handleController (midiChannel, controllerNumber, controllerValue);
}

void Synth::handleProgramChange (int midiChannel, int programNumber)
{
    selectionCache_.program = programNumber;
    setProgramSelection(selectionCache_);
}

bool Synth::hasProgramSelectionChanged (bool reset)
{
    // resets value after queried
    if (selectionChanged.get())
    {
        if (reset)
            selectionChanged.set(0);
        return true;
    }
    return false;
}

void Synth::noteOn (int midiChannel,
                    int midiNoteNumber,
                    float velocity)
{
    int i;
    
    const ScopedLock locker(lock);
    
    int midiVelocity = static_cast<int>(velocity * 127);
    
    // First, stop any currently-playing sounds in the group.
    //*** Currently, this only pays attention to the first matching region.
    int group = 0;
    Sound* sound = getSound();
    
    if (sound)
    {
        Region *region = sound->getRegionFor(midiNoteNumber, midiVelocity);
        if (region)
        {
            group = region->group;
        }
    }
    if (group != 0)
    {
        for (i = voices.size(); --i >= 0;)
        {
            Voice *voice = dynamic_cast<Voice *>(voices.getUnchecked(i));
            if (voice == nullptr)
            {
                continue;
            }
            if (voice->getOffBy() == group)
            {
                voice->stopNoteForGroup();
            }
        }
    }
    
    // Are any notes playing?  (Needed for first/legato trigger handling.)
    // Also stop any voices still playing this note.
    bool anyNotesPlaying = false;
    for (i = voices.size(); --i >= 0;)
    {
        Voice *voice = dynamic_cast<Voice *>(voices.getUnchecked(i));
        if (voice == nullptr)
        {
            continue;
        }
        if (voice->isPlayingChannel(midiChannel))
        {
            if (voice->isPlayingNoteDown())
            {
                if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
                {
                    if (!voice->isPlayingOneShot())
                    {
                        voice->stopNoteQuick();
                    }
                }
                else
                {
                    anyNotesPlaying = true;
                }
            }
        }
    }
    
    // Play *all* matching regions.
    Region::Trigger trigger = (anyNotesPlaying ? Region::legato : Region::first);
    if (sound)
    {
        int numRegions = sound->getNumRegions();
        for (i = 0; i < numRegions; ++i)
        {
            Region *region = sound->regionAt(i);
            if (region->matches(midiNoteNumber, midiVelocity, trigger))
            {
                Voice *voice =
                dynamic_cast<Voice *>(findFreeVoice(sound, midiNoteNumber, midiChannel, isNoteStealingEnabled()));
                if (voice)
                {
                    voice->setRegion(sound, region);
                    startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
                }
            }
        }
    }
    
    noteVelocities_[midiNoteNumber] = midiVelocity;
}

void Synth::noteOff (int midiChannel,
                     int midiNoteNumber,
                     float velocity,
                     bool allowTailOff)
{
    const ScopedLock locker(lock);
    
    Synthesiser::noteOff (midiChannel, midiNoteNumber, velocity, allowTailOff);
    
    // Start release region.
    Sound* sound = getSound();
    if (sound)
    {
        Region *region = sound->getRegionFor(midiNoteNumber, noteVelocities_[midiNoteNumber], Region::release);
        if (region)
        {
            Voice *voice = dynamic_cast<Voice *>(findFreeVoice(sound, midiNoteNumber, midiChannel, false));
            if (voice)
            {
                // Synthesiser is too locked-down (ivars are private rt protected), so
                // we have to use a "setRegion()" mechanism.
                voice->setRegion(sound, region);
                startVoice(voice, sound, midiChannel, midiNoteNumber, noteVelocities_[midiNoteNumber] / 127.0f);
            }
        }
    }
}

void Synth::renderVoices (AudioSampleBuffer &outputAudio, int startSample, int numSamples)
{
    for (int i = voices.size(); --i >= 0;)
        voices.getUnchecked (i)->renderNextBlock (outputAudio, startSample, numSamples);
    
    // Master Volume & Pan
    outputAudio.applyGain (0, startSample, numSamples, masterVolume_.get() * masterPanL_.get());
    outputAudio.applyGain (1, startSample, numSamples, masterVolume_.get() * masterPanR_.get());
    
    // Sidechain: Reverb send outputs
    jassert(outputAudio.getNumChannels() == 4);
    const float send = sendLevel_.get();
    outputAudio.copyFromWithRamp(2, 0, outputAudio.getReadPointer(0), numSamples, send, send);
    outputAudio.copyFromWithRamp(3, 0, outputAudio.getReadPointer(1), numSamples, send, send);
}

int Synth::numVoicesUsed()
{
    int numUsed = 0;
    
    for (int i = voices.size(); --i >= 0;)
    {
        if (voices.getUnchecked(i)->getCurrentlyPlayingNote() >= 0)
        {
            numUsed += 1;
        }
    }
    return numUsed;
}

String Synth::voiceInfoString()
{
    enum
    {
        maxShownVoices = 20,
    };
    
    StringArray lines;
    int numUsed = 0, numShown = 0;
    for (int i = voices.size(); --i >= 0;)
    {
        Voice *voice = dynamic_cast<Voice *>(voices.getUnchecked(i));
        if (voice->getCurrentlyPlayingNote() < 0)
        {
            continue;
        }
        numUsed += 1;
        if (numShown >= maxShownVoices)
        {
            continue;
        }
        lines.add(voice->infoString());
    }
    lines.insert(0, "voices used: " + String(numUsed));
    return lines.joinIntoString("\n");
}



std::unique_ptr<XmlElement> Synth::getStateXML ()
{
    auto xml = std::make_unique<XmlElement> ("SYNTH");
    xml->setAttribute ("slot",  channel_-1);
    // File and patch selection is handled by the parent (owner)
    // CC and raw values are saved to be robust against future changes in MIDI or UI handling
    xml->setAttribute("volume-cc", masterVolumeCC_.get());
    xml->setAttribute("volume",    masterVolume_.get());
    xml->setAttribute("pan-cc",    masterPanCC_.get());
    xml->setAttribute("pan-left",  masterPanL_.get());
    xml->setAttribute("pan-right", masterPanR_.get());
    xml->setAttribute("send-cc",   sendLevelCC_.get());
    xml->setAttribute("send",      sendLevel_.get());
    return xml;
}

bool Synth::setStateXML (const XmlElement* xml)
{
    if ((xml == nullptr) || !xml->hasTagName ("SYNTH"))
        return false;
    
    // Channel is a fixed assignment that must match!
    if ((channel_-1) != xml->getIntAttribute ("slot", 0))
        return false;
    
    masterVolumeCC_.set(xml->getIntAttribute("volume-cc"));
    masterVolume_.set(xml->getDoubleAttribute("volume"));
    masterPanCC_.set(xml->getIntAttribute("pan-cc"));
    masterPanL_.set(xml->getDoubleAttribute("pan-left"));
    masterPanR_.set(xml->getDoubleAttribute("pan-right"));
    sendLevelCC_.set(xml->getIntAttribute("send-cc"));
    sendLevel_.set(xml->getDoubleAttribute("send"));
    
    return true;
}

