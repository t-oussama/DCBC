#ifndef WORKER_MANAGER
#define WORKER_MANAGER

#include "../../common/net/connection/connection.h"
#include "../../common/net/socket/socket.h"
#include "../JobContext/JobContext.h"
#include "../WorkerHandler/WorkerHandler.h"
#include <thread>
#include <string>
#include <map>

using namespace std;
class WorkerManager
{
    private:
        Connection* conn;
        // Connection* whConn;
        mutex* connMutex;
        mutex* socketMutex;
        Socket* socket;
        map<string, WorkerHandler*> workerHandlers;
        JobContext* jobContext;
        WorkerManager* next;
        int currentWorkerIndex = -1;

    public:
        string id;
        WorkerManager(Socket* socket, mutex* socketMutex, Connection* conn, const int currentWorkerIndex, const string& id);
        void SetNextWorkerManager(WorkerManager* worker);
        void SetJobContext(JobContext* jc);
        void SetPreviousSalt(const char* salt, const char* chunkId);
        
        void EncryptChunk(const int chunkIndex);
        // void EncryptChunk(const string& chunkId, const char* chunk, const size_t size);
        void DecryptChunk(const int chunkIndex, const char* salt);

        ~WorkerManager();
};

#endif
