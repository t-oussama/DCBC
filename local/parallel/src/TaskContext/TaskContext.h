#ifndef TASK_CONTEXT
#define TASK_CONTEXT

#include <mutex>
#include <string>
#include <vector>
#include "../ChunkHandler/ChunkHandler.h"


using namespace std;
class TaskContext
{
    public:
        int id;
        int localChunk;
        int* remainingPageChunks;
        mutex* remainingPageChunksRW;
        unsigned int pageSlot;
        int pageSize;
        vector<string*> execTimeLog;
};
#endif