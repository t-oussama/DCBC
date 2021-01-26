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
        string outputDir;
        const char* HOST;
        int PORT;

        void connectToWorkerHandler();

    public:
        ChunkHandler(const char* host, const int port);
        void Encrypt(char* chunkId);
        void Decrypt(char* buffer, const int dataSize, char* chunkId);

        void encrypt(char* chunkId);
        void decrypt(char* chunk, char* chunkId);
        void SignalResetDone();
        void SignalDone();
};

#endif