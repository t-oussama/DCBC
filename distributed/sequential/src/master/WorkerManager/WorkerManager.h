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
        mutex* connMutex;
        mutex* socketMutex;
        Socket* socket;
        map<string, WorkerHandler*> workerHandlers;
        JobContext* jobContext;

    public:
        string id;
        WorkerManager(Socket* socket, mutex* socketMutex, Connection* conn, string id);
        void SetJobContext(JobContext* jc);
        void SaveCipher(const int chunkIndex);
        void GetChunk(const int chunkIndex);
        ~WorkerManager();
};

#endif
