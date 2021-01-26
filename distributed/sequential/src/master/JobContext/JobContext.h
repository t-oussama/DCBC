#ifndef JOB_CONTEXT
#define JOB_CONTEXT

#include <string>
#include <mutex>
#include <map>
#include <fstream>
#include <condition_variable>

using namespace std;
class JobContext
{
    public:
        const string* outputDir;
        const size_t* CHUNK_SIZE;
        size_t* fullClearTextSize;
        const string* fileId;
        char* fullClearText;
        mutex* chunkCountMutex;
        condition_variable* allDone;
        int* progress;
        int* chunkCount;
};

#endif