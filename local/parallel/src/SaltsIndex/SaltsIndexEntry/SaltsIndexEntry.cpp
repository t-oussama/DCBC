#include "SaltsIndexEntry.h"

SaltsIndexEntry::SaltsIndexEntry(unsigned char* value)
{
    this->value = value;
    this->isReady = false;
}

SaltsIndexEntry::~SaltsIndexEntry()
{
    delete this->value;
}