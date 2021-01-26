#ifndef SALTS_INDEX_ENTRY
#define SALTS_INDEX_ENTRY

#include <mutex>
#include <condition_variable>

using namespace std;

class SaltsIndexEntry
{
    public:
        unsigned char* value;
        bool isReady;
        mutex isReadyMutex;

        SaltsIndexEntry(unsigned char* value);

        ~SaltsIndexEntry();
};

#endif
