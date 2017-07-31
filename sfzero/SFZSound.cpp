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

#include "SFZSound.h"
#include "SFZReader.h"
#include "SFZRegion.h"
#include "SFZSample.h"
#include "SFZSharedResources.h"

using namespace juce;
using namespace sfzero;

Sound::Sound (const File &fileIn, int channel) :
    SynthesiserSound(),
    channel_(channel),
    selection_(0,0),
    file_(fileIn)
{
    selection_.name = fileIn.getFileName();
}

Sound::~Sound()
{
    int numRegions = regions_.size();
    for (int i = 0; i < numRegions; ++i)
    {
        delete regions_[i];
        regions_.set(i, nullptr);
    }
    sfzSamples_ = nullptr;
}

SharedResourcesSFZ* Sound::sharedSamples()
{
    if (sfzSamples_ == nullptr)
        sfzSamples_ = SharedResources::getInstance()->sfzResources(file_);
    
    return sfzSamples_;
}


bool Sound::appliesToNote(int /*midiNoteNumber*/)
{
    // Just say yes; we can't truly know unless we're told the velocity as well.
    return true;
}

bool Sound::appliesToChannel (int midiChannel)
{
    return midiChannel == channel_ || channel_ == 255;
}

void Sound::addRegion(Region *region)
{
    regions_.add(region);
}

Sample *Sound::addSample (String path, String defaultPath)
{
    path = path.replaceCharacter('\\', '/');
    defaultPath = defaultPath.replaceCharacter('\\', '/');
    File sampleFile;
    if (defaultPath.isEmpty())
    {
        sampleFile = file_.getSiblingFile(path);
    }
    else
    {
        File defaultDir = file_.getSiblingFile(defaultPath);
        sampleFile = defaultDir.getChildFile(path);
    }
    String samplePath = sampleFile.getFullPathName();
    Sample *sample = sharedSamples()->getSample(samplePath);
    if (sample == nullptr)
    {
        sample = new Sample(sampleFile);
        sharedSamples()->setSample(samplePath, sample);
    }
    return sample;
}

void Sound::addError(const String &message) { errors_.add(message); }

void Sound::addUnsupportedOpcode(const String &opcode)
{
    if (!unsupportedOpcodes_.contains(opcode))
    {
        unsupportedOpcodes_.set(opcode, opcode);
        String warning = "unsupported opcode: ";
        warning << opcode;
        warnings_.add(warning);
    }
}

void Sound::loadRegions()
{
    Reader reader(this);
    
    reader.read(file_);
}

void Sound::loadSamples (AudioFormatManager *formatManager,
                         double *progressVar,
                         Thread *thread)
{
    sharedSamples()->loadSamples (this, formatManager, progressVar, thread);
}

Region *Sound::getRegionFor (int note, int velocity, Region::Trigger trigger)
{
    int numRegions = regions_.size();
    
    for (int i = 0; i < numRegions; ++i)
    {
        Region *region = regions_[i];
        if (region->matches(note, velocity, trigger))
        {
            return region;
        }
    }
    return nullptr;
}

int Sound::getNumRegions()
{
    return regions_.size();
}

Region *Sound::regionAt (int index)
{
    return regions_[index];
}



ProgramSelection& Sound::getProgramSelection ()
{
    // returns always the same selection
    return selection_;
}

void Sound::setProgramSelection (const ProgramSelection& selection)
{
    // this has no effect here: only 1 fixed program available
}

int Sound::getProgramCount (int bank)
{
    // banks don't exist here
    return 1;
}

String Sound::getProgramName (const ProgramSelection& selection)
{
    // returns always the same selection
    return selection_.name;
}

ProgramList* Sound::getProgramList()
{
    // only 1 program available
    ProgramList* list = new ProgramList();
    list->add (new ProgramSelection(selection_));
    return list;
}



String Sound::dump()
{
    String info;
    auto &errors = getErrors();
    if (errors.size() > 0)
    {
        info << errors.size() << " errors: \n";
        info << errors.joinIntoString("\n");
        info << "\n";
    }
    else
    {
        info << "no errors.\n\n";
    }
    
    auto &warnings = getWarnings();
    if (warnings.size() > 0)
    {
        info << warnings.size() << " warnings: \n";
        info << warnings.joinIntoString("\n");
    }
    else
    {
        info << "no warnings.\n";
    }
    
    if (regions_.size() > 0)
    {
        info << regions_.size() << " regions: \n";
        for (int i = 0; i < regions_.size(); ++i)
        {
            info << regions_[i]->dump();
        }
    }
    else
    {
        info << "no regions.\n";
    }
    
    info << sharedSamples()->dump();
    
    return info;
}
