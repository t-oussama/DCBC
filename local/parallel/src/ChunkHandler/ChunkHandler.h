#ifndef CHUNK_HANDLER
#define CHUNK_HANDLER
#include <thread>
#include <map>
#include <string>
#include <mutex>
#include "../JobContext/JobContext.h"

#define OP_ENC 1
#define OP_DEC 0

using namespace std;

class JobContext;


class ChunkHandler
{
    private:
        thread* t;
        mutex* threadBarrier;
        int currentTask;
        unsigned int id;
        JobContext* jc;
        unsigned char* plainTextKey;

        void generateSalt(unsigned char* hash, const unsigned char* previousSalt, unsigned char* salt);
        void phase2(char* clearText, int dataSize, char* salt, char* previousSalt, char* chunkId, string chunkIndex);
        void setPreviousFlag(char* buffer, char* chunkId);
        void connectToWorkerHandler();
        void _encrypt();
        void _decrypt();

    public:
        ChunkHandler(unsigned int id, JobContext* jc, bool op);
        ~ChunkHandler();

        void runTask(int taskIndex);
        void handle(bool op);
        void join();
};

#endif