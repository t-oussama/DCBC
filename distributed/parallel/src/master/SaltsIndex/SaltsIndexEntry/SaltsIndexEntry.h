#ifndef SALTS_INDEX_ENTRY
#define SALTS_INDEX_ENTRY

#include <mutex>

using namespace std;

class SaltsIndexEntry
{
    public:
        char* value;
        bool isRequested;
        mutex writeMutex;
        mutex requestMutex;

        SaltsIndexEntry(char* value);

        ~SaltsIndexEntry();
};

#endif
