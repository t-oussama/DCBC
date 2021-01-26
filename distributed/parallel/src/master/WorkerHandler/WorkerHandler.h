#ifndef WORKER_HANDLER
#define WORKER_HANDLER

#include "../../common/net/connection/connection.h"
#include "../../common/net/socket/socket.h"
#include "../JobContext/JobContext.h"
#include "../SaltsIndex/SaltsIndex.h"
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
        WorkerManager** next;
        int chunkIndex = -1;
        char* salt;
        
        void handleWorker();
        void onGetPreviousSalt(char* buffer);
        void onSetSalt(char* buffer);
        void onChunkHandled();
        void onChunkSent();
        void onChunkReset();

    public:
        WorkerHandler(const int chunkIndex, Socket* socket, mutex* socketMutex, JobContext* jc, WorkerManager** next);
        void UseSalt(char* salt);
        void SetPreviousSalt(const char* salt);
        void Reset();
        void setOutputDir();

        ~WorkerHandler();
};

#endif
