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
        vector<WorkerManager*>* workers;
        map<string, int> workersIds;
        mutex chunkCountMutex;
        int chunkCount;
        condition_variable allDone;
        char* fullClearText;
        size_t fullClearTextSize = 0;
        thread* connectionHandler;
        int min_workers;

        void dispatchJobContext(JobContext* jc);
    public:
        JobManager(const int min_workers);
        int ClusterSize();
        void HandleConnection();
        void Encrypt(const string filePath, const string outputDir, const size_t CHUNK_SIZE);
        void Decrypt(string filePath, const string outputDir, const size_t CHUNK_SIZE);
        ~JobManager();
};

#endif
