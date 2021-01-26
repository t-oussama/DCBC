#include "WorkerManager.h"
#include <string>
#include <thread>
#include "../JobContext/JobContext.h"
#include <iostream>
#include "../../common/net/socket/socket.h"
#include "../WorkerManager/WorkerManager.h"
#include "../../common/constants/comm-flags.h"
#include "../WorkerHandler/WorkerHandler.h"
#include <fstream>

#define CHUNK_ID_LENGTH 5

using namespace std;

WorkerManager::WorkerManager(Socket* socket, mutex* socketMutex, Connection* conn, const int currentWorkerIndex, const string& id)
{
    this->conn = conn;
    this->id = id;
    this->socketMutex = socketMutex;
    this->socket = socket;
    // char* buffer;

    this->jobContext = NULL;
    this->currentWorkerIndex = currentWorkerIndex;
}

void WorkerManager::EncryptChunk(const int chunkIndex)
{
    const string chunkId = *this->jobContext->fileId + char(chunkIndex + 2);
    WorkerHandler* workerHandler = new WorkerHandler(chunkIndex, this->socket, this->socketMutex, this->jobContext, &this->next);
    this->workerHandlers.insert({chunkId, workerHandler});
    this->conn->Send(chunkId.c_str(), ENCRYPT_CHUNK_FLAG, CHUNK_ID_LENGTH);
}

void WorkerManager::DecryptChunk(const int chunkIndex, const char* salt)
{
    const string chunkId = *this->jobContext->fileId + char(chunkIndex + 2);
    const char* data[2];
    data[0] = chunkId.c_str();
    data[1] = salt;
    size_t sizes[2] = {CHUNK_ID_LENGTH, 16};
    WorkerHandler* workerHandler = new WorkerHandler(chunkIndex, this->socket, this->socketMutex, this->jobContext, &this->next);
    this->workerHandlers.insert({chunkId, workerHandler});
    this->conn->SendMany(data, DECRYPT_CHUNK_FLAG, sizes, 2);
}

void WorkerManager::SetJobContext(JobContext* jc)
{
    this->jobContext = jc;
}

void WorkerManager::SetNextWorkerManager(WorkerManager* worker)
{
    this->next = worker;
}

void WorkerManager::SetPreviousSalt(const char* salt, const char* chunkId)
{
    WorkerHandler* workerHandler = this->workerHandlers.find(string(chunkId))->second;
    workerHandler->SetPreviousSalt(salt);
}

WorkerManager::~WorkerManager()
{
    for(auto it = this->workerHandlers.begin(); it != this->workerHandlers.end(); ++it)
    {
        delete it->second;
    }
    this->workerHandlers.clear();
}
