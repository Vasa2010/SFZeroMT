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

#include "SF2Generator.h"

using namespace sfzero;

#define SF2GeneratorValue(name, type)                                                                                            \
  {                                                                                                                              \
    #name, SF2Generator::type                                                                                            \
  }

static const sfzero::SF2Generator generators[] = {
#include "sf2-chunks/generators.h"
};

#undef SF2GeneratorValue

const SF2Generator* sfzero::GeneratorFor(int index)
{
  static const int numGenerators = sizeof(generators) / sizeof(generators[0]);

  if (index >= numGenerators)
  {
    return nullptr;
  }
  return &generators[index];
}
