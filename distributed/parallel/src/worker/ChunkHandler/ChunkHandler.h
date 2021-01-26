#ifndef CHUNK_HANDLER
#define CHUNK_HANDLER
#include <thread>
#include <map>
#include <string>
#include <mutex>
#include "../../common/net/connection/connection.h"

#define CHUNK_ID_LENGTH  5

using namespace std;


class ChunkHandler
{
    private:
        thread* t;
        Connection* conn;
        string workerId;
        string outputDir;
        const char* HOST;
        int PORT;

        char* input;
        // mutex threadBarrier;

        void combine(char* salt, const char* previousSalt);
        void phase2(char* clearText, int dataSize, char* salt, char* previousSalt, char* chunkId, string chunkIndex);
        void setPreviousFlag(char* buffer, char* chunkId);
        void connectToWorkerHandler();

    public:
        ChunkHandler(const char* host, const int port);
        void Encrypt(char* chunkId);
        void Decrypt(char* buffer, const int dataSize, char* chunkId);

        void encrypt(char* chunkId);
        void decrypt(char* chunk, char* chunkId);
        // void setBuffer(char* buffer, char* chunkId);
        void handleChunk(char* chunk, const int dataSize, char* chunkId);
        
        void RequestPreviousSalt(const char* currentChunkId);
        void SignalResetDone();
        void SignalDone();
};

#endif