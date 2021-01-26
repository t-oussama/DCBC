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

// WorkerHandler::WorkerHandler(Socket* socket, mutex* socketMutex, const int chunkIndex)
WorkerHandler::WorkerHandler(const int chunkIndex, Socket* socket, mutex* socketMutex, JobContext* jc, WorkerManager** next)
{
    this->socketMutex = socketMutex;
    this->socket = socket;
    this->jobContext = jc;
    this->chunkIndex = chunkIndex;
    this->t = new thread(&WorkerHandler::handleWorker, this);
    this->t->detach();
    this->next = next;
    this->whConn = NULL;
}

void WorkerHandler::UseSalt(char* salt)
{
    this->whConn->Send(salt, SET_SALT_FLAG, 16);
}

void WorkerHandler::SetPreviousSalt(const char* salt)
{
    this->whConn->Send(salt, GET_PREV_SALT_RESP_FLAG, 16);
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
        if (buffer[0] == SET_SALT_FLAG)
        {
            this->onSetSalt(buffer);
        }
        else if (buffer[0] == GET_PREV_SALT_FLAG)
        {
            this->onGetPreviousSalt(buffer);
        }
        else if (buffer[0] == SIGNAL_DONE_FLAG)
        {
            this->onChunkHandled();
        }
        else if (buffer[0] == SIGNAL_RESET_DONE_FLAG)
        {
            this->onChunkReset();
        }
        else if (buffer[0] == SET_CHUNK_FLAG)
        {
            // int chunkIndex = (int) buffer[CHUNK_ID_LENGTH];
            int dataLength = size - 1;
            // *(this->jobContext->fullClearTextSize) += dataLength;
            // memcpy(this->jobContext->fullClearText + this->chunkIndex * *(this->jobContext->CHUNK_SIZE), buffer + 1, dataLength);
            this->jobContext->outputFileMutex->lock();
            this->jobContext->outputFile->seekp(this->chunkIndex * *(this->jobContext->CHUNK_SIZE));
            this->jobContext->outputFile->write(buffer + 1, dataLength);
            this->jobContext->outputFileMutex->unlock();
            this->onChunkHandled();
        }
        else if (buffer[0] == GET_CHUNK_FLAG)
        {
            char* chunk = *this->jobContext->fullClearText + (this->chunkIndex % *this->jobContext->pageSize) * *(this->jobContext->CHUNK_SIZE);
            this->whConn->Send(chunk, ENCRYPT_CHUNK_FLAG, *(this->jobContext->CHUNK_SIZE));
            this->onChunkSent();
        }
        delete buffer;
        buffer = NULL;
    }
}

void WorkerHandler::onGetPreviousSalt(char* buffer)
{
    if (this->chunkIndex == 0) // if first chunk
    {
        char* iv = "1234567890123456";
        this->SetPreviousSalt(iv);
    }
    else
    {
        char* salt = this->jobContext->saltsIndex->get(this->chunkIndex - 1);
        if (!salt)
        {
            this->jobContext->saltsIndex->request(this->chunkIndex - 1);
        }
        else
        {
            this->SetPreviousSalt(salt);
        }
    }
}

void WorkerHandler::onSetSalt(char* buffer)
{
    string nextChunkId = *this->jobContext->fileId + char(this->chunkIndex + 3);
    char* salt = new char[16];
    memcpy(salt, buffer + 1, 16);
    this->jobContext->saltsIndex->set(this->chunkIndex, salt);
    if (this->jobContext->saltsIndex->isRequested(this->chunkIndex))
    {
        // If the salt is requested, that means next woker is alreading running (as he is the one who made the request) so we know that this->next exists !
        (*this->next)->SetPreviousSalt(salt, nextChunkId.c_str());
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

void WorkerHandler::onChunkSent()
{
this->jobContext->pageCountMutex->lock();
    *(this->jobContext->pageProgress) += 1;
    // cout << *(this->jobContext->pageProgress) << " / " << *(this->jobContext->pageSize) << endl;
    this->jobContext->pageCountMutex->unlock();
    if (*(this->jobContext->pageProgress) == *(this->jobContext->pageSize))
    {
        this->jobContext->pageDispatched->notify_all();
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
