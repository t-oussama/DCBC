#ifndef JOB_CONTEXT
#define JOB_CONTEXT

#include <mutex>
#include <string>
#include <vector>
#include <fstream>
#include "../ChunkHandler/ChunkHandler.h"
#include "../TaskContext/TaskContext.h"
#include "../SaltsIndex/SaltsIndex.h"
#include "openssl/aes.h"

using namespace std;

class ChunkHandler;

class JobContext
{
    public:
        unsigned int* pendingChunksIndex;
        mutex* pendingChunksIndexRW;
        TaskContext** tasks;
        vector<unsigned int>* availableChunkHandlers;
        mutex* availableChunkHandlersRW;
        string outputDir;
        char** i_pages;
        char** o_pages;
        unsigned int totalPages;
        unsigned int chunkCount;
        int* pagesProgress;
        mutex* pagesProgressRW;
        mutex pageRW;
        int i_file;
        int o_file;
        int* loadedTasks;
        ChunkHandler** chPool;
        mutex* tasksRW;
        unsigned int* pendingTask;
        mutex* pendingTaskRW;
        SaltsIndex* saltsIndex;
        char* output;
        const int* PAGE_SIZE;
        const size_t* CHUNK_SIZE;
        unsigned char* dataEncryptionKey;
        unsigned char* saltEncryptionKey;
        
};
#endif