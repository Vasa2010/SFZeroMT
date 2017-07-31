# SFZeroMT

## SFZero Juce Module for Multi-Timbral Operation

This work is an extension of [SFZero Juce module by Leo Olivers](https://github.com/altalogix/SFZeroModule), based on [SFZero by Steve Folta](https://github.com/stevefolta/SFZero), with the following extensions and changes:

* Support multi-timbral operation (multiple instances on different MIDI channels)
* Support MIDI program & bank selection message
* Support master volume & pan, reset controllers and similar MIDI CC 
* Load sample data only once, shared by all instances
* Handle SF2 global zone generators, e.g. initialAttenuation
* Save/restore Synth state with XML
* Bug fixes and streamlining

SFZeroMT requires [Juce](http://www.juce.com) version 4.3 or later.

## Usage

SFZeroMT can be used for projects that require multiple instances of a SFZero Synth be run at the same time, while sharing a common pool of samples to reduce memory footprint: 

* Per each MIDI channel, integrate into an AudioProcessor one instance of sfzero::Synth
* Have processBlock() do synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples())
* Arrange these processors in an AudioProcessorGraph
* Feed the graph with MIDI input

In theory, it is possible to load a different SF2 file per channel, but this has not been tested. Standard operation is to have all synths load the same SF2 file, so they can share its sample data. How to load a SF2 file:

``` 
auto sound = new sfzero::SF2Sound(file, channel);  
sound->loadRegions();  
sound->loadSamples(&formatManager, progressVar, thread);  
synth.swapSound(sound);  
```

Shared memory management works by reference counting. So if a sound is no longer used by any Synth, it will be deleted. Note that the term 'Sound' is a bit misleading here, as a SF2 file actually consists of many sounds, each of which is selected by a bank and program change MIDI message.

## Project Status

This fork was worked on as a side project, without putting much effort into porting it to our standard coding and documentation norms. Anyone familiar with Juce should be able to figure out its workings easily.

