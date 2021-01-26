#ifndef JOB_CONTEXT
#define JOB_CONTEXT

#include <string>
#include <mutex>
#include <map>
#include <fstream>
#include <condition_variable>
#include "../SaltsIndex/SaltsIndex.h"

using namespace std;
class JobContext
{
    public:
        const string* outputDir;
        const size_t* CHUNK_SIZE;
        size_t* fullClearTextSize;
        const string* fileId;
        char** fullClearText;
        SaltsIndex* saltsIndex;
        mutex* chunkCountMutex;
        mutex* pageCountMutex;
        condition_variable* allDone;
        int* progress;
        int* chunkCount;
        condition_variable* pageDispatched;
        int* pageProgress;
        int* pageSize;

        ofstream* outputFile;
        mutex* outputFileMutex;
};

#endif