#ifndef SALTS_INDEX
#define SALTS_INDEX

#include "./SaltsIndexEntry/SaltsIndexEntry.h"

using namespace std;

class SaltsIndex
{
    private:
        SaltsIndexEntry** index;
        int size;

    public:
        SaltsIndex(const int size);
        void set(const int chunkIndex, unsigned char* value);
        void add(const int chunkIndex);
        bool isReady(const int chunkIndex);
        unsigned char* get(const int chunkIndex);
        ~SaltsIndex();
};

#endif
