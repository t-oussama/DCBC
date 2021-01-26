#include "WorkerHandler.h"
#include <string>
#include <thread>
#include "../JobContext/JobContext.h"
#include <iostream>
#include "../../common/net/socket/socket.h"
#include "../WorkerHandler/WorkerHandler.h"
#include "../../common/constants/comm-flags.h"
#include "../WorkerManager/WorkerManager.h"

#define CHUNK_ID_LENGTH 5

using namespace std;

WorkerHandler::WorkerHandler(const int chunkIndex, Socket* socket, mutex* socketMutex, JobContext* jc)
{
    this->socketMutex = socketMutex;
    this->socket = socket;
    this->jobContext = jc;
    this->chunkIndex = chunkIndex;
    this->t = new thread(&WorkerHandler::handleWorker, this);
    this->t->detach();
    this->whConn = NULL;
}

void WorkerHandler::Reset()
{
    char* message = "\0";
    this->whConn->Send(message, RESET_FLAG, 1);
}

void WorkerHandler::setOutputDir()
{
    this->whConn->Send(this->jobContext->outputDir->c_str(), SET_OUTPUT_DIR_FLAG, this->jobContext->outputDir->length());
}

// Private

void WorkerHandler::handleWorker()
{
    this->socketMutex->lock();
    this->whConn = this->socket->Accept();
    this->socketMutex->unlock();
    this->setOutputDir();

    char* buffer;

    while (true)
    {
        int size = this->whConn->Recv(&buffer);

        if (buffer[0] == SIGNAL_DONE_FLAG)
        {
            this->onChunkHandled();
        }
        else if (buffer[0] == SIGNAL_RESET_DONE_FLAG)
        {
            this->onChunkReset();
        }
        else if (buffer[0] == SET_CHUNK_FLAG)
        {
            int dataLength = size - 1;
            cout << "Recieved Cipher Chunk #" << this->chunkIndex << ", Size: " << dataLength << endl;
            *(this->jobContext->fullClearTextSize) += dataLength;
            memcpy(this->jobContext->fullClearText + this->chunkIndex * *(this->jobContext->CHUNK_SIZE), buffer + 1, dataLength);
            this->onChunkHandled();
        }
        else if (buffer[0] == GET_CHUNK_FLAG)
        {
            char* chunk = this->jobContext->fullClearText + this->chunkIndex * *(this->jobContext->CHUNK_SIZE);
            this->whConn->Send(chunk, ENCRYPT_CHUNK_FLAG, *(this->jobContext->CHUNK_SIZE));
        }
        delete buffer;
        buffer = NULL;
    }
}

void WorkerHandler::onChunkHandled()
{
this->jobContext->chunkCountMutex->lock();
    *(this->jobContext->progress) += 1;
    cout << *(this->jobContext->progress) << " / " << *(this->jobContext->chunkCount) << endl;
    this->jobContext->chunkCountMutex->unlock();
    if (*(this->jobContext->progress) == *(this->jobContext->chunkCount))
    {
        this->jobContext->allDone->notify_all();
    }
}

void WorkerHandler::onChunkReset()
{
    this->jobContext->chunkCountMutex->lock();
    *(this->jobContext->progress) += 1;
    this->jobContext->chunkCountMutex->unlock();
    this->jobContext->allDone->notify_all();
}

WorkerHandler::~WorkerHandler()
{
    delete this->t;
}
