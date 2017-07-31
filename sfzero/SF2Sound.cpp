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

#include "SF2Sound.h"
#include "SF2Reader.h"
#include "SFZSample.h"
#include "SFZSharedResources.h"

using namespace juce;
using namespace sfzero;


SF2Sound::SF2Sound (const File &fileIn, int channel) :
    Sound (fileIn, channel)
{
}

SF2Sound::~SF2Sound()
{
    // "presets_" owns the regions, so clear them out of "regions" so ~SFZSound() doesn't try to delete them.
    getRegions().clear();
    
    HashMap<int, Preset*>::Iterator i (presets_);
    while (i.next())
    {
        delete i.getValue();
    }
    presets_.clear();
    
    sf2Samples_ = nullptr;
}



SharedResourcesSF2* SF2Sound::sharedSamples()
{
    if (sf2Samples_ == nullptr)
        sf2Samples_ = SharedResources::getInstance()->sf2Resources(file_);
    
    return sf2Samples_;
}

void SF2Sound::addPreset (Preset *preset)
{
    presets_.set(preset->index(), preset);
}

void SF2Sound::loadRegions()
{
    SF2Reader reader(this, getFile());
    reader.read();
    setProgramSelection(ProgramSelection());
}

void SF2Sound::loadSamples(AudioFormatManager *formatManager, double *progressVar, Thread *thread)
{
    sharedSamples()->loadSamples(this, formatManager, progressVar, thread);
}


ProgramSelection& SF2Sound::getProgramSelection ()
{
    // returns the current selection incl. name
    return selection_;
}

void SF2Sound::setProgramSelection (const ProgramSelection& selection)
{
    // It is important to call this under a lock from Synth.
    // Unused programs can be selected, but will produce no sound.
    
    selection_ = selection;
    
    Preset* preset = presets_[selection.index()];
    if (preset)
    {
        getRegions().clear();
        getRegions().addArray(preset->regions);
        selection_.name = preset->getName();
        
    } else {
        getRegions().clear();
        selection_.name = String::empty;
    }
}

int SF2Sound::getProgramCount (int bank)
{
    // Always returns 128 (MIDI program numbers)
    // Non-existent banks simply deliver empty sound names
    return 128;
}

String SF2Sound::getProgramName (const ProgramSelection& selection)
{
    // Returns an empty default name for all unused program numbers
    Preset* preset = presets_[selection.index()];
    if (preset)
        return preset->getName();
    else
        return String::empty;
}

ProgramList* SF2Sound::getProgramList()
{
    // Return a full list of all available ProgramSelections
    // The ordering of this list is NOT related to MIDI program numbers
    
    ProgramList* list = new ProgramList();
    HashMap<int, Preset*>::Iterator i (presets_);
    while (i.next())
    {
        list->add (new ProgramSelection (i.getValue()->getSelection()));
    }
    ProgramSelectionComparator comparator;
    list->sort(comparator);
    return list;
}


bool SF2Sound::hasBank (int bank)
{
    HashMap<int, Preset*>::Iterator i (presets_);
    while (i.next())
    {
        ProgramSelection& selection = i.getValue()->getSelection();
        if (selection.bank == bank)
            return true;
    }
    return false;
}


Sample *SF2Sound::sampleFor(double sampleRate)
{
    return sharedSamples()->getSample(sampleRate);
}

