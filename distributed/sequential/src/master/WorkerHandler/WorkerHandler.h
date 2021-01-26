#ifndef WORKER_HANDLER
#define WORKER_HANDLER

#include "../../common/net/connection/connection.h"
#include "../../common/net/socket/socket.h"
#include "../JobContext/JobContext.h"
// #include "../WorkerManager/WorkerManager.h"
#include <thread>
#include <string>
#include <vector>

class WorkerManager;

using namespace std;
class WorkerHandler
{
    private:
        Connection* whConn;
        mutex* socketMutex;
        Socket* socket;
        JobContext* jobContext;
        thread* t;
        int chunkIndex = -1;
        
        void handleWorker();
        void onChunkHandled();
        void onChunkReset();

    public:
        WorkerHandler(const int chunkIndex, Socket* socket, mutex* socketMutex, JobContext* jc);
        void Reset();
        void setOutputDir();

        ~WorkerHandler();
};

#endif
