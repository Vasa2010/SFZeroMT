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

#include "SFZSharedResources.h"
#include "SFZDebug.h"
#include "SF2Reader.h"


/*********************************************************************************
 *    SharedResourceBase
 *********************************************************************************/

sfzero::SharedResourceBase::SharedResourceBase (juce::String filename) :
    lock_ (),
    filename_ (filename),
    loaded_ (false),
    used_ (false)
{
}

sfzero::SharedResourceBase::~SharedResourceBase ()
{
}

/*********************************************************************************
 *    SharedResourcesSFZ
 *********************************************************************************/

sfzero::SharedResourcesSFZ::SharedResourcesSFZ (juce::String filename) :
    SharedResourceBase(filename),
    samples_()
{
}

sfzero::SharedResourcesSFZ::~SharedResourcesSFZ ()
{
    DBG ("Deleting SharedResourcesSFZ " << getKey());
    if (SharedResources::getInstanceWithoutCreating() != nullptr)
        SharedResources::getInstance()->sfzRemove(getKey());
    
    // All samples share the same buffer, so make sure only the last will be deleted below
    juce::AudioSampleBuffer *buffer = nullptr;
    for (juce::HashMap<juce::String, sfzero::Sample *>::Iterator i(samples_); i.next();)
    {
        buffer = i.getValue()->detachBuffer();
    }
    delete buffer;
    for (juce::HashMap<juce::String, sfzero::Sample *>::Iterator i(samples_); i.next();)
    {
        delete i.getValue();
    }
}

sfzero::Sample* sfzero::SharedResourcesSFZ::getSample (const juce::String name)
{
    juce::ScopedLock sl (lock_);
    return samples_[name];
}

sfzero::Sample* sfzero::SharedResourcesSFZ::setSample (const juce::String name, sfzero::Sample* sample)
{
    juce::ScopedLock sl (lock_);
    samples_.set(name, sample);
    return sample;
}

void sfzero::SharedResourcesSFZ::loadSamples (sfzero::Sound *sound,
                                              juce::AudioFormatManager *formatManager,
                                              double *progressVar,
                                              juce::Thread *thread)
{
    juce::ScopedLock sl (lock_);
    
    // Load samples only once
    if (!loaded_)
    {
        if (progressVar)
            *progressVar = 0.0;
        
        double numSamplesLoaded = 1.0, numSamples = samples_.size();
        for (juce::HashMap<juce::String, Sample*>::Iterator i(samples_); i.next();)
        {
            sfzero::Sample *sample = i.getValue();
            bool ok = sample->load(formatManager);
            if (!ok)
                sound->addError("failed loading sample \"" + sample->getShortName() + "\"");
            
            numSamplesLoaded += 1.0;
            if (progressVar)
                *progressVar = numSamplesLoaded / numSamples;
            
            if (thread && thread->threadShouldExit())
                return;
        }
        loaded_ = true;
        
    } else {
        sound->addUnsupportedOpcode("using shared samples");
    }
    if (progressVar)
        *progressVar = 1.0;
}


juce::String sfzero::SharedResourcesSFZ::dump()
{
    juce::ScopedLock sl (lock_);
    juce::String info;
    
    if (samples_.size() > 0)
    {
        info << samples_.size() << " samples: \n";
        for (juce::HashMap<juce::String, sfzero::Sample *>::Iterator i(samples_); i.next();)
        {
            info << i.getValue()->dump();
        }
    }
    else
    {
        info << "no samples.\n";
    }
    return info;
}


/*********************************************************************************
 *    SharedResourcesSF2
 *********************************************************************************/


sfzero::SharedResourcesSF2::SharedResourcesSF2 (juce::String filename) :
    SharedResourceBase(filename),
    samplesByRate_ ()
{
}

sfzero::SharedResourcesSF2::~SharedResourcesSF2 ()
{
    DBG("Deleting SharedResourcesSF2 " << getKey());
    
    if (SharedResources::getInstanceWithoutCreating() != nullptr)
        SharedResources::getInstance()->sf2Remove(getKey());
    
    // All samples share the same buffer, so make sure only the last will be deleted below
    juce::AudioSampleBuffer *buffer = nullptr;
    for (juce::HashMap<int, sfzero::Sample*>::Iterator i(samplesByRate_); i.next();)
    {
        buffer = i.getValue()->detachBuffer();
    }
    delete buffer;
    // Now delete all detached samples
    for (juce::HashMap<int, sfzero::Sample*>::Iterator i(samplesByRate_); i.next();)
    {
        delete i.getValue();
    }
#if JUCE_DEBUG
    for (juce::HashMap<int, juce::String*>::Iterator i(sampleNamesByOffset_); i.next();)
    {
        delete i.getValue();
    }
#endif
}



void sfzero::SharedResourcesSF2::loadSamples (sfzero::SF2Sound *sound,
                                              juce::AudioFormatManager *formatManager,
                                              double *progressVar,
                                              juce::Thread *thread)
{
    juce::ScopedLock sl (lock_);
    
    // Load samples only once
    if (!loaded_)
    {
        sfzero::SF2Reader reader(sound, sound->getFile());
        juce::AudioSampleBuffer *buffer = reader.readSampleData(progressVar, thread);
        
        if (buffer)
        {
            // All Samples share the same buffer
            for (juce::HashMap<int, sfzero::Sample *>::Iterator i(samplesByRate_); i.next();)
            {
                i.getValue()->setBuffer(buffer);
            }
        }
        loaded_ = true;
        
    } else {
        sound->addUnsupportedOpcode("using shared samples");
    }
    if (progressVar)
        *progressVar = 1.0;
}

sfzero::Sample* sfzero::SharedResourcesSF2::getSample (double sampleRate)
{
    juce::ScopedLock sl (lock_);
    
    sfzero::Sample *sample = samplesByRate_[static_cast<int>(sampleRate)];
    if (sample == nullptr)
    {
        sample = new sfzero::Sample(sampleRate);
        samplesByRate_.set(static_cast<int>(sampleRate), sample);
    }
    return sample;
}


/*********************************************************************************
 *    SharedResources
 *********************************************************************************/

juce_ImplementSingleton (sfzero::SharedResources)

sfzero::SharedResources::SharedResources () :
    lock_ (),
    sfz_ (),
    sf2_ ()
{
}

sfzero::SharedResources::~SharedResources ()
{
    DBG("Deleting SharedResources");
    clearSingletonInstance();
}

sfzero::SharedResourcesSFZ* sfzero::SharedResources::sfzResources (const juce::File& filename)
{
    juce::ScopedLock sl (lock_);
    
    juce::String fn (filename.getFullPathName());
    SharedResourcesSFZ* samples = sfz_[fn];
    if (samples == nullptr)
    {
        samples = new SharedResourcesSFZ(fn);
        sfz_.set(fn, samples);
    }
    return samples;
}

sfzero::SharedResourcesSF2* sfzero::SharedResources::sf2Resources (const juce::File& filename)
{
    juce::ScopedLock sl (lock_);
    
    juce::String fn (filename.getFullPathName());
    SharedResourcesSF2* samples = sf2_[fn];
    if (samples == nullptr)
    {
        samples = new SharedResourcesSF2(fn);
        sf2_.set(fn, samples);
    }
    return samples;
}

void sfzero::SharedResources::sfzRemove (const juce::File& filename)
{
    juce::ScopedLock sl (lock_);
    sfz_.remove (filename.getFullPathName());
}

void sfzero::SharedResources::sf2Remove (const juce::File& filename)
{
    juce::ScopedLock sl (lock_);
    sf2_.remove (filename.getFullPathName());
}






