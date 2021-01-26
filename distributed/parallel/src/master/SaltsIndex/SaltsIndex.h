#ifndef SALTS_INDEX
#define SALTS_INDEX

#include "./SaltsIndexEntry/SaltsIndexEntry.h"

using namespace std;

class SaltsIndex
{
    private:
        // map<const string, SaltsIndexEntry*> index;
        SaltsIndexEntry** index;
        int size;

    public:
        SaltsIndex(const int size);
        void set(const int chunkIndex, char* value);
        void add(const int chunkIndex);
        void request(const int chunkIndex);
        bool isRequested(const int chunkIndex);
        char* get(const int chunkIndex);
        ~SaltsIndex();
};

#endif
