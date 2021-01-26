#include "SaltsIndex.h"
#include <map>
#include <mutex>
#include <string>

SaltsIndex::SaltsIndex(const int size)
{
    this->index = new SaltsIndexEntry*[size];
    this->size = size;
}

void SaltsIndex::set(const int chunkIndex, char* value)
{
    this->index[chunkIndex]->writeMutex.lock();
    this->index[chunkIndex]->value = value;
    this->index[chunkIndex]->writeMutex.unlock();
}

void SaltsIndex::add(const int chunkIndex)
{
    this->index[chunkIndex] = new SaltsIndexEntry(NULL);
}

void SaltsIndex::request(const int chunkIndex)
{
    SaltsIndexEntry* entry = this->index[chunkIndex];
    entry->requestMutex.lock();
    entry->isRequested = true;
    entry->requestMutex.unlock();
}

bool SaltsIndex::isRequested(const int chunkIndex)
{
    return this->index[chunkIndex]->isRequested;
}

char* SaltsIndex::get(const int chunkIndex)
{
    return this->index[chunkIndex]->value;
}

SaltsIndex::~SaltsIndex()
{
    for(int i = 0; i < this->size; i++)
    {
        delete this->index[i];
    }
    delete this->index;
}
