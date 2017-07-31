/*
BEGIN_JUCE_MODULE_DECLARATION
    ID:               SFZeroMT
    vendor:           cognitone
    version:          1.0.0
    name:             SFZeroMT
    description:      Multi-timbral extension of SFZero by Steve Folta, converted to Juce module by Leo Olivers and extended by Cognitone. Run multiple Synth instances, sharing sample data in shared memory.
    website:          https://github.com/cognitone/SFZeroMT
    dependencies:     juce_gui_basics, juce_audio_basics, juce_audio_processors
    license:          MIT
END_JUCE_MODULE_DECLARATION 
*/

#ifndef INCLUDED_SFZEROMT_H
#define INCLUDED_SFZEROMT_H

#include "sfzero/RIFF.h"
#include "sfzero/SF2.h"
#include "sfzero/SF2Generator.h"
#include "sfzero/SF2Reader.h"
#include "sfzero/SF2Sound.h"
#include "sfzero/SF2WinTypes.h"
#include "sfzero/SFZCommon.h"
#include "sfzero/SFZDebug.h"
#include "sfzero/SFZEG.h"
#include "sfzero/SFZReader.h"
#include "sfzero/SFZRegion.h"
#include "sfzero/SFZSample.h"
#include "sfzero/SFZSound.h"
#include "sfzero/SFZSynth.h"
#include "sfzero/SFZVoice.h"

#include "sfzero/SFZExtensions.h"
#include "sfzero/SFZSharedResources.h"


#endif   // INCLUDED_SFZEROMT_H

