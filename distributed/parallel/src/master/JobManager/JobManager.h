#ifndef NODE_HANDLER
#define NODE_HANDLER

#include "../../common/net/socket/socket.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>
#include <thread>
#include "../WorkerManager/WorkerManager.h"
#include "../SaltsIndex/SaltsIndex.h"
// #include "../JobContext/JobContext.h"

class JobContext;

using namespace std;
class JobManager
{
    private:
        Socket connectionSocket;
        Socket tasksSocket;
        mutex tasksSocketMutex;
        int progress;
        mutex allDoneMutex;
        SaltsIndex* saltsIndex;
        // map<const string, char*> saltsIndex;
        // mutex saltsIndexMutex;
        vector<WorkerManager*>* workers;
        map<string, int> workersIds;
        mutex chunkCountMutex;
        int chunkCount;
        condition_variable allDone;
        int pageProgress;
        int pageSize;
        mutex pageDispatchedMutex;
        condition_variable pageDispatched;
        mutex pageCountMutex;
        char* fullClearText;
        size_t fullClearTextSize = 0;
        thread* connectionHandler;
        int min_workers;

        void dispatchJobContext(JobContext* jc);
    public:
        JobManager(const int min_workers, int pageSize);
        int ClusterSize();
        void HandleConnection();
        void Encrypt(const string filePath, const string outputDir, const size_t CHUNK_SIZE);
        void Decrypt(string filePath, const string outputDir, const size_t CHUNK_SIZE);
        ~JobManager();


    private:
        void EncryptChunk(const std::string& chunkId, const char* chunk, const size_t size);
        void DecryptChunk(const std::string& chunkId, const char* salt);
};

#endif
