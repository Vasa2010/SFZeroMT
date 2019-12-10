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

#include "SF2Reader.h"
#include "RIFF.h"
#include "SF2.h"
#include "SF2Generator.h"
#include "SF2Sound.h"

using namespace juce;
using namespace sfzero;

SF2Reader::SF2Reader(SF2Sound *soundIn, const File &fileIn) :
    sound_(soundIn),
    file_(fileIn.createInputStream())
{
}

SF2Reader::~SF2Reader()
{
}

void SF2Reader::read()
{
    if (file_ == nullptr)
    {
        sound_->addError("Couldn't open file.");
        return;
    }
    // Read the hydra.
    SF2::Hydra hydra;
    file_->setPosition(0);
    RIFFChunk riffChunk;
    riffChunk.readFrom(file_.get());
    while (file_->getPosition() < riffChunk.end())
    {
        RIFFChunk chunk;
        chunk.readFrom(file_.get());
        if (FourCCEquals(chunk.id, "pdta"))
        {
            hydra.readFrom(file_.get(), chunk.end());
            break;
        }
        chunk.seekAfter(file_.get());
    }
    if (!hydra.isComplete())
    {
        sound_->addError("Invalid SF2 file (missing or incomplete hydra).");
        return;
    }
    
    // Read each preset.
    for (int whichPreset = 0; whichPreset < hydra.phdrNumItems - 1; ++whichPreset)
    {
        SF2::phdr *phdr = &hydra.phdrItems[whichPreset];
        Preset *preset = new Preset(phdr->presetName, phdr->bank, phdr->preset);
        sound_->addPreset(preset);
        
        // Zones
        /** @todo Handle more global zone generators/modulators than initialAttentuation */
        
        int firstPresetZone = phdr->presetBagNdx;
        int zoneEnd = phdr[1].presetBagNdx;
        for (int whichZone = phdr->presetBagNdx; whichZone < zoneEnd; ++whichZone)
        {
            SF2::pbag *pbag = &hydra.pbagItems[whichZone];
            Region presetRegion;
            presetRegion.clearForRelativeSF2();
            
            // Generators.
            int genEnd = pbag[1].genNdx;
            for (int whichGen = pbag->genNdx; whichGen < genEnd; ++whichGen)
            {
                SF2::pgen *pgen = &hydra.pgenItems[whichGen];
                
                if (whichZone == firstPresetZone && pgen->genOper == SF2Generator::initialAttenuation)
                {
                    //DBG(preset->getName() << " global initialAttenuation=" << pgen->genAmount.shortAmount);
                    presetRegion.volume = pgen->genAmount.shortAmount * SF2_INITIAL_ATTENUATION_TO_DB;
                }
                
                // Instrument.
                if (pgen->genOper == SF2Generator::instrument)
                {
                    word whichInst = pgen->genAmount.wordAmount;
                    if (whichInst < hydra.instNumItems)
                    {
                        Region instRegion;
                        instRegion.clearForSF2();
                        // Preset generators are supposed to be "relative" modifications of
                        // the instrument settings, but that makes no sense for ranges.
                        // For those, we'll have the instrument's generator take
                        // precedence, though that may not be correct.
                        instRegion.lokey = presetRegion.lokey;
                        instRegion.hikey = presetRegion.hikey;
                        instRegion.lovel = presetRegion.lovel;
                        instRegion.hivel = presetRegion.hivel;
                        
                        SF2::inst *inst = &hydra.instItems[whichInst];
                        int firstZone = inst->instBagNdx;
                        int zoneEnd2 = inst[1].instBagNdx;
                        for (int whichZone2 = firstZone; whichZone2 < zoneEnd2; ++whichZone2)
                        {
                            SF2::ibag *ibag = &hydra.ibagItems[whichZone2];
                            
                            // Generators.
                            Region zoneRegion = instRegion;
                            bool hadSampleID = false;
                            int genEnd2 = ibag[1].instGenNdx;
                            
                            for (int whichGen2 = ibag->instGenNdx; whichGen2 < genEnd2; ++whichGen2)
                            {
                                SF2::igen *igen = &hydra.igenItems[whichGen2];
                                if (igen->genOper == SF2Generator::sampleID)
                                {
                                    int whichSample = igen->genAmount.wordAmount;
                                    SF2::shdr *shdr = &hydra.shdrItems[whichSample];
                                    zoneRegion.addForSF2(&presetRegion);
                                    zoneRegion.sf2ToSFZ();
                                    zoneRegion.offset += shdr->start;
                                    zoneRegion.end += shdr->end;
                                    zoneRegion.loop_start += shdr->startLoop;
                                    zoneRegion.loop_end += shdr->endLoop;
                                    if (shdr->endLoop > 0)
                                    {
                                        zoneRegion.loop_end -= 1;
                                    }
                                    if (zoneRegion.pitch_keycenter == -1)
                                    {
                                        zoneRegion.pitch_keycenter = shdr->originalPitch;
                                    }
                                    zoneRegion.tune += shdr->pitchCorrection;                                    
                                    
                                    // Pin initialAttenuation to max +6dB.
                                    if (zoneRegion.volume > 6.0)
                                    {
                                        zoneRegion.volume = 6.0;
                                        sound_->addUnsupportedOpcode("extreme gain in initialAttenuation");
                                    }
                                    
                                    Region *newRegion = new Region();
                                    *newRegion = zoneRegion;
                                    newRegion->sample = sound_->sampleFor(shdr->sampleRate);
                                    preset->addRegion(newRegion);
                                    hadSampleID = true;
                                }
                                else
                                {
                                    addGeneratorToRegion(igen->genOper, &igen->genAmount, &zoneRegion);
                                }
                            }
                            
                            // Handle instrument's global zone.
                            if ((whichZone2 == firstZone) && !hadSampleID)
                            {
                                instRegion = zoneRegion;
                            }
                            
                            // Modulators.
                            int modEnd = ibag[1].instModNdx;
                            int whichMod = ibag->instModNdx;
                            if (whichMod < modEnd)
                            {
                                sound_->addUnsupportedOpcode("any modulator");
                            }
                        }
                    }
                    else
                    {
                        sound_->addError("Instrument out of range.");
                    }
                }
                // Other generators.
                else
                {
                    addGeneratorToRegion(pgen->genOper, &pgen->genAmount, &presetRegion);
                }
            }
            
            // Modulators.
            int modEnd = pbag[1].modNdx;
            int whichMod = pbag->modNdx;
            if (whichMod < modEnd)
            {
                sound_->addUnsupportedOpcode("any modulator");
            }
        }
    }
#if JUCE_DEBUG
    // Debug: Register names of individual SF2 samples by offset in sample data chunk
    for (int whichSample = 0; whichSample < hydra.shdrNumItems - 1; ++whichSample)
    {
        SamplePosition offset = hydra.shdrItems[whichSample].start;
        juce::String name (hydra.shdrItems[whichSample].sampleName, 20);
        sound_->sharedSamples()->sampleNameAtPut(offset, name);
        //DBG ("offset=" << offset << " name=" << name);
    }
#endif
}

AudioSampleBuffer *SF2Reader::readSampleData (double *progressVar, Thread *thread)
{
    if (file_ == nullptr)
    {
        sound_->addError("Couldn't open file.");
        return nullptr;
    }
    
    // Find the "sdta" chunk.
    file_->setPosition(0);
    RIFFChunk riffChunk;
    riffChunk.readFrom(file_.get());
    bool found = false;
    RIFFChunk chunk;
    while (file_->getPosition() < riffChunk.end())
    {
        chunk.readFrom(file_.get());
        if (FourCCEquals(chunk.id, "sdta"))
        {
            found = true;
            break;
        }
        chunk.seekAfter(file_.get());
    }
    int64 sdtaEnd = chunk.end();
    found = false;
    while (file_->getPosition() < sdtaEnd)
    {
        chunk.readFrom(file_.get());
        if (FourCCEquals(chunk.id, "smpl"))
        {
            found = true;
            break;
        }
        chunk.seekAfter(file_.get());
    }
    if (!found)
    {
        sound_->addError("SF2 is missing its \"smpl\" chunk.");
        return nullptr;
    }
    
    /* Note: In standard SF2 format, all samples are 16-bit uncompressed (short),
     saved in a single chunk of data. Sample's meta data (loops) refer directly
     to offsets within this chunk. This makes loading the samples a snap,
     however, it does not support compressed formats like SF3, which would
     require each sample to be located and loaded individually.
     */
    
    static const int bufferSize = 128000;
    int numSamples = (int)chunk.size / sizeof(short);
    AudioSampleBuffer *sampleBuffer = new AudioSampleBuffer(1, numSamples);
    //sound_->addError(String(numSamples) + " samples");
    
    // Read and convert in small chunks, so progress bar can be updated
    int   samplesLeft = numSamples;
    float *out = sampleBuffer->getWritePointer(0);
    ScopedPointer<short> buffer = new short[bufferSize];
    
    while (samplesLeft > 0)
    {
        // Read the buffer.
        int samplesToRead = bufferSize;
        if (samplesToRead > samplesLeft)
        {
            samplesToRead = samplesLeft;
        }
        file_->read(buffer, samplesToRead * sizeof(short));
        
        // Convert from signed 16-bit to float.
        int samplesToConvert = samplesToRead;
        short *in = buffer;
        for (; samplesToConvert > 0; --samplesToConvert)
        {
            // If we ever need to compile for big-endian platforms, we'll need to byte-swap here.
            *out++ = *in++ / 32767.0f;
        }
        samplesLeft -= samplesToRead;
        
        if (progressVar)
        {
            *progressVar = static_cast<float>(numSamples - samplesLeft) / numSamples;
        }
        if (thread && thread->threadShouldExit())
        {
            delete sampleBuffer;
            return nullptr;
        }
    }
    
    if (progressVar)
    {
        *progressVar = 1.0;
    }
    return sampleBuffer;
}


void SF2Reader::addGeneratorToRegion (word genOper, SF2::genAmountType *amount, Region *region)
{
    switch (genOper)
    {
        case SF2Generator::startAddrsOffset:
            region->offset += amount->shortAmount;
            break;
            
        case SF2Generator::endAddrsOffset:
            region->end += amount->shortAmount;
            break;
            
        case SF2Generator::startloopAddrsOffset:
            region->loop_start += amount->shortAmount;
            break;
            
        case SF2Generator::endloopAddrsOffset:
            region->loop_end += amount->shortAmount;
            break;
            
        case SF2Generator::startAddrsCoarseOffset:
            region->offset += amount->shortAmount * 32768;
            break;
            
        case SF2Generator::endAddrsCoarseOffset:
            region->end += amount->shortAmount * 32768;
            break;
            
        case SF2Generator::pan:
            region->pan = amount->shortAmount * (2.0f / 10.0f);
            break;
            
        case SF2Generator::delayVolEnv:
            region->ampeg.delay = amount->shortAmount;
            break;
            
        case SF2Generator::attackVolEnv:
            region->ampeg.attack = amount->shortAmount;
            break;
            
        case SF2Generator::holdVolEnv:
            region->ampeg.hold = amount->shortAmount;
            break;
            
        case SF2Generator::decayVolEnv:
            region->ampeg.decay = amount->shortAmount;
            break;
            
        case SF2Generator::sustainVolEnv:
            region->ampeg.sustain = amount->shortAmount;
            break;
            
        case SF2Generator::releaseVolEnv:
            region->ampeg.release = amount->shortAmount;
            break;
            
        case SF2Generator::keyRange:
            region->lokey = amount->range.lo;
            region->hikey = amount->range.hi;
            break;
            
        case SF2Generator::velRange:
            region->lovel = amount->range.lo;
            region->hivel = amount->range.hi;
            break;
            
        case SF2Generator::startloopAddrsCoarseOffset:
            region->loop_start += amount->shortAmount * 32768;
            break;
            
        case SF2Generator::initialAttenuation:
            region->volume += amount->shortAmount * SF2_INITIAL_ATTENUATION_TO_DB;
            break;
            
        case SF2Generator::endloopAddrsCoarseOffset:
            region->loop_end += amount->shortAmount * 32768;
            break;
            
        case SF2Generator::coarseTune:
            region->transpose += amount->shortAmount;
            break;
            
        case SF2Generator::fineTune:
            region->tune += amount->shortAmount;
            break;
            
        case SF2Generator::sampleModes:
        {
            Region::LoopMode loopModes[] = {Region::no_loop, Region::loop_continuous, Region::no_loop,
                Region::loop_sustain};
            region->loop_mode = loopModes[amount->wordAmount & 0x03];
            break;
        }
            
        case SF2Generator::scaleTuning:
            region->pitch_keytrack = amount->shortAmount;
            break;
            
        case SF2Generator::exclusiveClass:
            region->off_by = amount->wordAmount;
            region->group = static_cast<int>(region->off_by);
            break;
            
        case SF2Generator::overridingRootKey:
            region->pitch_keycenter = amount->shortAmount;
            break;
            
        case SF2Generator::endOper:
            // Ignore.
            break;
            
        case SF2Generator::modLfoToPitch:
        case SF2Generator::vibLfoToPitch:
        case SF2Generator::modEnvToPitch:
        case SF2Generator::initialFilterFc:
        case SF2Generator::initialFilterQ:
        case SF2Generator::modLfoToFilterFc:
        case SF2Generator::modEnvToFilterFc:
        case SF2Generator::modLfoToVolume:
        case SF2Generator::unused1:
        case SF2Generator::reverbEffectsSend:
        case SF2Generator::chorusEffectsSend:
        case SF2Generator::unused2:
        case SF2Generator::unused3:
        case SF2Generator::unused4:
        case SF2Generator::delayModLFO:
        case SF2Generator::freqModLFO:
        case SF2Generator::delayVibLFO:
        case SF2Generator::freqVibLFO:
        case SF2Generator::delayModEnv:
        case SF2Generator::attackModEnv:
        case SF2Generator::holdModEnv:
        case SF2Generator::decayModEnv:
        case SF2Generator::sustainModEnv:
        case SF2Generator::releaseModEnv:
        case SF2Generator::keynumToModEnvHold:
        case SF2Generator::keynumToModEnvDecay:
        case SF2Generator::keynumToVolEnvHold:
        case SF2Generator::keynumToVolEnvDecay:
        case SF2Generator::instrument:
            // Only allowed in certain places, where we already special-case it.
        case SF2Generator::reserved1:
        case SF2Generator::keynum:
        case SF2Generator::velocity:
        case SF2Generator::reserved2:
        case SF2Generator::sampleID:
            // Only allowed in certain places, where we already special-case it.
        case SF2Generator::reserved3:
        case SF2Generator::unused5:
        {
            const SF2Generator *generator = GeneratorFor(static_cast<int>(genOper));
            sound_->addUnsupportedOpcode(generator->name);
        }
            break;
    }
}
