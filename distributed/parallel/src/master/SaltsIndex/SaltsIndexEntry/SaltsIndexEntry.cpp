#include "SaltsIndexEntry.h"

SaltsIndexEntry::SaltsIndexEntry(char* value)
{
    this->value = value;
    this->isRequested = false;
}

SaltsIndexEntry::~SaltsIndexEntry()
{
    delete this->value;
}