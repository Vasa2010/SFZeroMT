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

#include "SFZDebug.h"
#include <stdarg.h>

#ifdef JUCE_DEBUG

void sfzero::dbgprintf(const char *msg, ...)
{
    va_list args;
    
    va_start(args, msg);
    
    char output[256];
    vsnprintf(output, 256, msg, args);
    
    va_end(args);
}

#endif // JUCE_DEBUG
