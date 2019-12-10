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

#ifndef SFZSHAREDRESOURCES_H_INCLUDED
#define SFZSHAREDRESOURCES_H_INCLUDED

#include "SFZCommon.h"
#include "SFZSample.h"

/*  SharedResourcesSFZ, SharedResourcesSF2 are global singeltons that hold
    sample data of a single SFZ/SF2 file that can be used by multiple 
    instances of Sound/SF2Sound */

namespace sfzero
{
    
    class Sound;
    class SF2Sound;
    
    class SharedResourceBase : public juce::ReferenceCountedObject
    {
    public:
        
        SharedResourceBase (juce::String filename);
        virtual ~SharedResourceBase ();
        
        juce::String& getKey() { return filename_; };
        
    protected:
        juce::CriticalSection lock_;
        juce::String filename_;
        bool loaded_;
        bool used_;
    };
    
    /*********************************************************************************
     *    SharedResourcesSFZ
     *********************************************************************************/
    
    class SharedResourcesSFZ : public SharedResourceBase
    {
    public:
        
        typedef juce::ReferenceCountedObjectPtr<SharedResourcesSFZ> Ptr;
        typedef juce::HashMap<juce::String, SharedResourcesSFZ*> Lookup;
        
        SharedResourcesSFZ (juce::String filename);
        virtual ~SharedResourcesSFZ ();
        
        Sample* getSample (const juce::String name);
        Sample* setSample (const juce::String name, Sample* sample);
        
        void loadSamples (Sound *sound,
                          juce::AudioFormatManager *formatManager,
                          double *progressVar,
                          juce::Thread *thread);
        
        juce::String dump();
        
    private:
        juce::HashMap<juce::String, Sample*> samples_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharedResourcesSFZ)
    };
    
    /*********************************************************************************
     *    SharedResourcesSF2
     *********************************************************************************/
    
    
    class SharedResourcesSF2 : public SharedResourceBase
    {
    public:
        
        typedef juce::ReferenceCountedObjectPtr<SharedResourcesSF2> Ptr;
        typedef juce::HashMap<juce::String, SharedResourcesSF2*> Lookup;
        
        SharedResourcesSF2 (juce::String filename);
        virtual ~SharedResourcesSF2 ();
        
        Sample* getSample (double sampleRate);
        
        void loadSamples (SF2Sound *sound,
                          juce::AudioFormatManager *formatManager,
                          double *progressVar,
                          juce::Thread *thread);

#if JUCE_DEBUG
        juce::String* sampleNameAt (SamplePosition offset)
            { return sampleNamesByOffset_[offset]; }
        
        void sampleNameAtPut (SamplePosition offset, const juce::String name)
            { sampleNamesByOffset_.set(offset, new juce::String(name)); }
        
    private:
        juce::HashMap<SamplePosition, juce::String*> sampleNamesByOffset_; // for debugging only
#endif
    private:
        juce::HashMap<int, Sample*> samplesByRate_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharedResourcesSF2)
    };
    
    /*********************************************************************************
     *    SharedResources
     *********************************************************************************/
    
    /** A global singelton serving as the keeper of shared sample resources */
    
    class SharedResources
    {
    public:
        SharedResources();
        ~SharedResources();
        
        juce_DeclareSingleton (SharedResources, false);
        
        SharedResourcesSFZ* sfzResources (const juce::File& filename);
        SharedResourcesSF2* sf2Resources (const juce::File& filename);
        
        void sfzRemove (const juce::File& filename);
        void sf2Remove (const juce::File& filename);
        
    private:
        juce::CriticalSection lock_;
        SharedResourcesSFZ::Lookup sfz_;
        SharedResourcesSF2::Lookup sf2_;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SharedResources)
    };
    
}

#endif // SFZSHAREDRESOURCES_H_INCLUDED
