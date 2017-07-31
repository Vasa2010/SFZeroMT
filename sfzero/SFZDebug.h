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

#ifndef SFZDEBUG_H_INCLUDED
#define SFZDEBUG_H_INCLUDED

#include "SFZCommon.h"

#ifdef JUCE_DEBUG

namespace sfzero
{
    void dbgprintf(const char *msg, ...);
}

#endif

#endif // SFZDEBUG_H_INCLUDED
