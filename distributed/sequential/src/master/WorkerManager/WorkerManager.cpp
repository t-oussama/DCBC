#include "WorkerManager.h"
#include "../../common/net/socket/socket.h"
#include "../WorkerManager/WorkerManager.h"
#include "../WorkerHandler/WorkerHandler.h"
#include "../JobContext/JobContext.h"
#include <string>

#include "../../common/constants/comm-flags.h"

#define CHUNK_ID_LENGTH 5

using namespace std;

WorkerManager::WorkerManager(Socket* socket, mutex* socketMutex, Connection* conn, string id)
{
    this->conn = conn;
    this->socketMutex = socketMutex;
    this->socket = socket;
    this->id = id;
    this->jobContext = NULL;
}

void WorkerManager::SaveCipher(const int chunkIndex)
{
    const string chunkId = *this->jobContext->fileId + char(chunkIndex + 2);
    WorkerHandler* workerHandler = new WorkerHandler(chunkIndex, this->socket, this->socketMutex, this->jobContext);
    this->workerHandlers.insert({chunkId, workerHandler});
    this->conn->Send(chunkId.c_str(), ENCRYPT_CHUNK_FLAG, CHUNK_ID_LENGTH);
}

void WorkerManager::GetChunk(const int chunkIndex)
{
    const string chunkId = *this->jobContext->fileId + char(chunkIndex + 2);
    WorkerHandler* workerHandler = new WorkerHandler(chunkIndex, this->socket, this->socketMutex, this->jobContext);
    this->workerHandlers.insert({chunkId, workerHandler});
    this->conn->Send(chunkId.c_str(), DECRYPT_CHUNK_FLAG, CHUNK_ID_LENGTH);
}

void WorkerManager::SetJobContext(JobContext* jc)
{
    this->jobContext = jc;
}

WorkerManager::~WorkerManager()
{
    for(auto it = this->workerHandlers.begin(); it != this->workerHandlers.end(); ++it)
    {
        delete it->second;
    }
    this->workerHandlers.clear();
}
