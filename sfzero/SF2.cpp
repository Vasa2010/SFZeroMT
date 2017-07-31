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

#include "SF2.h"
#include "RIFF.h"

using namespace juce;
using namespace sfzero;

#define readAbyte(name, file) 	\
        name = (byte) file->readByte();
#define readAchar(name, file) 	\
        name = file->readByte();
#define readAdword(name, file) 	\
        name = (dword) file->readInt();
#define readAword(name, file) 	\
        name = (word) file->readShort();
#define readAshort(name, file) 	\
        name = file->readShort();
#define readAchar20(name, file) 	\
        file->read(name, 20);
#define readAgenAmountType(name, file) 	\
        name.shortAmount = file->readShort();

#define SF2Field(type, name) 	\
        readA##type(name, file)

void SF2::iver::readFrom(InputStream *file)
{
#include "sf2-chunks/iver.h"
}

void SF2::phdr::readFrom(InputStream *file)
{
#include "sf2-chunks/phdr.h"
}

void SF2::pbag::readFrom(InputStream *file)
{
#include "sf2-chunks/pbag.h"
}

void SF2::pmod::readFrom(InputStream *file)
{
#include "sf2-chunks/pmod.h"
}

void SF2::pgen::readFrom(InputStream *file)
{
#include "sf2-chunks/pgen.h"
}

void SF2::inst::readFrom(InputStream *file)
{
#include "sf2-chunks/inst.h"
}

void SF2::ibag::readFrom(InputStream *file)
{
#include "sf2-chunks/ibag.h"
}

void SF2::imod::readFrom(InputStream *file)
{
#include "sf2-chunks/imod.h"
}

void SF2::igen::readFrom(InputStream *file)
{
#include "sf2-chunks/igen.h"
}

void SF2::shdr::readFrom(InputStream *file)
{
#include "sf2-chunks/shdr.h"
}

SF2::Hydra::Hydra() :
    phdrItems(nullptr),
    pbagItems(nullptr),
    pmodItems(nullptr),
    pgenItems(nullptr),
    instItems(nullptr),
    ibagItems(nullptr),
    imodItems(nullptr),
    igenItems(nullptr),
    shdrItems(nullptr),
    phdrNumItems(0),
    pbagNumItems(0),
    pmodNumItems(0),
    pgenNumItems(0),
    instNumItems(0),
    ibagNumItems(0),
    imodNumItems(0),
    igenNumItems(0),
    shdrNumItems(0)
{
}

SF2::Hydra::~Hydra()
{
    delete phdrItems;
    delete pbagItems;
    delete pmodItems;
    delete pgenItems;
    delete instItems;
    delete ibagItems;
    delete imodItems;
    delete igenItems;
    delete shdrItems;
}

void SF2::Hydra::readFrom(InputStream *file, int64 pdtaChunkEnd)
{
    int i, numItems;
    
	#define HandleChunk(chunkName) 	\
		if (FourCCEquals(chunk.id, #chunkName)) { 	\
			numItems = chunk.size / SF2::chunkName::sizeInFile; 	\
			chunkName##NumItems = numItems; 	\
			chunkName##Items = new SF2::chunkName[numItems]; 	\
			for (i = 0; i < numItems; ++i) 	\
				chunkName##Items[i].readFrom(file); 	\
			} 	\
		else
    
    while (file->getPosition() < pdtaChunkEnd)
    {
        RIFFChunk chunk;
        chunk.readFrom(file);
        
        HandleChunk(phdr)
        HandleChunk(pbag)
        HandleChunk(pmod)
        HandleChunk(pgen)
        HandleChunk(inst)
        HandleChunk(ibag)
        HandleChunk(imod)
        HandleChunk(igen)
        HandleChunk(shdr)
        { }
        
        chunk.seekAfter(file);
    }
}

bool SF2::Hydra::isComplete()
{
    return phdrItems && pbagItems && pmodItems && pgenItems && instItems && ibagItems && imodItems && igenItems && shdrItems;
}
