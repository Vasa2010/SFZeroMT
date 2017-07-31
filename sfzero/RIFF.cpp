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

#include "RIFF.h"

using namespace juce;
using namespace sfzero;

void RIFFChunk::readFrom(InputStream *file)
{
    file->read(&id, sizeof(fourcc));
    size = static_cast<dword>(file->readInt());
    start = file->getPosition();
    
    if (FourCCEquals(id, "RIFF"))
    {
        type = RIFF;
        file->read(&id, sizeof(fourcc));
        start += sizeof(fourcc);
        size -= sizeof(fourcc);
    }
    else if (FourCCEquals(id, "LIST"))
    {
        type = LIST;
        file->read(&id, sizeof(fourcc));
        start += sizeof(fourcc);
        size -= sizeof(fourcc);
    }
    else
    {
        type = Custom;
    }
}

void RIFFChunk::seek(InputStream *file) { file->setPosition(start); }
void RIFFChunk::seekAfter(InputStream *file)
{
    int64 next = start + size;
    
    if (next % 2 != 0)
    {
        next += 1;
    }
    file->setPosition(next);
}

String RIFFChunk::readString(InputStream *file)
{
    MemoryBlock memoryBlock(size);
    file->read(memoryBlock.getData(), static_cast<int>(memoryBlock.getSize()));
    return memoryBlock.toString();
}
