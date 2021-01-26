#include "SaltsIndex.h"
#include <map>
#include <mutex>
#include <string>
#include <iostream>

SaltsIndex::SaltsIndex(const int size)
{
    this->index = new SaltsIndexEntry*[size];
    this->size = size;
    for(int i = 0; i < size; i++)
    {
        this->add(i);
    }
}


void SaltsIndex::add(const int chunkIndex)
{
    this->index[chunkIndex] = new SaltsIndexEntry(NULL);
    this->index[chunkIndex]->isReadyMutex.lock();
}

void SaltsIndex::set(const int chunkIndex, unsigned char* value)
{
    unsigned char* newVal = new unsigned char[16];
    for(int i = 0; i < 16; i++)
    {
        newVal[i] = value[i];
    }
    this->index[chunkIndex]->value = newVal;
    this->index[chunkIndex]->isReady = true;
    this->index[chunkIndex]->isReadyMutex.unlock();
}

bool SaltsIndex::isReady(const int chunkIndex)
{
    if (chunkIndex < 0)
    {
        return true;
    }
    this->index[chunkIndex]->isReadyMutex.lock();
    return this->index[chunkIndex]->isReady;
}

unsigned char* SaltsIndex::get(const int chunkIndex)
{
    if (chunkIndex >= 0)
    {
        return this->index[chunkIndex]->value;
    }
    
    return (unsigned char*) "1234567890123456";
}

SaltsIndex::~SaltsIndex()
{
    for(int i = 0; i < this->size; i++)
    {
        delete this->index[i];
    }
    delete this->index;
}
