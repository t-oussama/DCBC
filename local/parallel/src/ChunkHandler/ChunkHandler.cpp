#include "ChunkHandler.h"
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>

// Crypto++ includes
// #include "openssl/sha.h"
#include "openssl/md5.h"
#include "openssl/aes.h"


// Just for logging
#include <chrono>

#include <sys/mman.h>


#define time std::chrono::high_resolution_clock

using namespace std;

ChunkHandler::ChunkHandler(unsigned int id, JobContext* jc, bool op)
{
    this->id = id;
    this->jc = jc;
    this->currentTask = -1;
    this->threadBarrier = new mutex();
    this->threadBarrier->lock();
    this->t = new thread(&ChunkHandler::handle, this, op);
    // this->t->detach();
}

void ChunkHandler::runTask(int taskIndex)
{
    this->currentTask = taskIndex;
    this->threadBarrier->unlock();
}

void ChunkHandler::handle(bool op)
{
    while(true)
    {
        this->threadBarrier->lock();
        if (this->currentTask < 0)
            break;
        // Encryption logic ...
        if(op == 1)
            this->_encrypt();
        else
            this->_decrypt();
        // It's ok for this lock to be over a big chunk of code: 
        // - in case the "if" condition is not satisfied, the op is significantly shorter so no problem there
        // - in case the "if" condition is satisfied, that means this is the last thread handling this page's chunks, and since pages have different mutex instances, locking this one doesn't affect anyone else
        this->jc->tasks[this->currentTask]->remainingPageChunksRW->lock();
        *(this->jc->tasks[this->currentTask]->remainingPageChunks) -= 1;
        // If last chunk in page
        if((*this->jc->tasks[this->currentTask]->remainingPageChunks) == 0)
        {
            // Unload page
            if (munmap(this->jc->i_pages[this->jc->tasks[this->currentTask]->pageSlot], (size_t) this->jc->tasks[this->currentTask]->pageSize * *this->jc->CHUNK_SIZE) == -1)
            {
                cerr << "Error while unmapping input page !" << endl;
            }

            delete this->jc->tasks[this->currentTask]->remainingPageChunks;
            this->jc->tasks[this->currentTask]->remainingPageChunksRW->unlock();
            delete this->jc->tasks[this->currentTask]->remainingPageChunksRW;

            // Load new page
            if (*this->jc->pagesProgress < this->jc->totalPages)
            {
                this->jc->i_pages[this->jc->tasks[this->currentTask]->pageSlot] = (char*) mmap(NULL, (size_t) *this->jc->PAGE_SIZE * *this->jc->CHUNK_SIZE, PROT_READ, MAP_PRIVATE, this->jc->i_file, (size_t) (*this->jc->pagesProgress) * *this->jc->PAGE_SIZE * *this->jc->CHUNK_SIZE);
                if (this->jc->i_pages[this->jc->tasks[this->currentTask]->pageSlot] == MAP_FAILED)
                {
                    cerr << "Mapping failed !" << endl;
                }

                int* remainingPageChunks = new int(*this->jc->PAGE_SIZE);
                mutex* remainingPageChunksRW = new mutex();

                this->jc->tasksRW->lock();
                for (int j = 0; j < *this->jc->PAGE_SIZE; j++)
                {
                    this->jc->tasks[*this->jc->loadedTasks] = new TaskContext();
                    this->jc->tasks[*this->jc->loadedTasks]->pageSlot = this->jc->tasks[this->currentTask]->pageSlot;
                    this->jc->tasks[*this->jc->loadedTasks]->localChunk = j;
                    this->jc->tasks[*this->jc->loadedTasks]->remainingPageChunks = remainingPageChunks;
                    this->jc->tasks[*this->jc->loadedTasks]->remainingPageChunksRW = remainingPageChunksRW;
                    this->jc->tasks[*this->jc->loadedTasks]->pageSize = *this->jc->PAGE_SIZE;
                    this->jc->pendingTaskRW->lock();
                    (*this->jc->loadedTasks) += 1;
                    this->jc->pendingTaskRW->unlock();
                }
                this->jc->tasksRW->unlock();
                this->jc->availableChunkHandlersRW->lock();
                this->jc->pendingTaskRW->lock();

                for(int i = 0; this->jc->availableChunkHandlers->size() && (*this->jc->pendingTask) < (*this->jc->loadedTasks); i++)
                {
                    const int id = this->jc->availableChunkHandlers->back();
                    this->jc->availableChunkHandlers->pop_back();
                    this->jc->chPool[id]->runTask(*this->jc->pendingTask);
                    (*this->jc->pendingTask) += 1;
                }
                this->jc->pendingTaskRW->unlock();
                this->jc->availableChunkHandlersRW->unlock();

                this->jc->pagesProgressRW->lock();
                (*this->jc->pagesProgress) += 1;
                this->jc->pagesProgressRW->unlock();
            }
        }
        else
        {
            this->jc->tasks[this->currentTask]->remainingPageChunksRW->unlock();
        }

        this->jc->pendingTaskRW->lock();
        // if there are more chunks to handle
        if (*this->jc->pendingTask < this->jc->chunkCount)
        {
            // if any of those tasks are loaded
            if ((*this->jc->pendingTask) < (*this->jc->loadedTasks))
            {
                unsigned int x = *this->jc->pendingTask;
                (*this->jc->pendingTask) += 1;
                this->jc->pendingTaskRW->unlock();
                this->runTask(x);
            }
            else
            {
                this->jc->pendingTaskRW->unlock();
                this->jc->availableChunkHandlersRW->lock();
                this->jc->availableChunkHandlers->push_back(this->id);
                this->jc->availableChunkHandlersRW->unlock();
            }
        }
        else
        {
            // Empty the waiting chunk handlers pool
            this->jc->availableChunkHandlersRW->lock();
            while(this->jc->availableChunkHandlers->size())
            {
                const int id = this->jc->availableChunkHandlers->back();
                this->jc->availableChunkHandlers->pop_back();
                this->jc->chPool[id]->runTask(-1);
            }
            this->jc->availableChunkHandlersRW->unlock();
            this->jc->pendingTaskRW->unlock();
            break ;
        }
    }
}

void ChunkHandler::_encrypt()
{
    const size_t dataSize = *this->jc->CHUNK_SIZE;
    char* chunk = this->jc->i_pages[this->jc->tasks[this->currentTask]->pageSlot] + (size_t) (this->jc->tasks[this->currentTask]->localChunk * *this->jc->CHUNK_SIZE);
    // char* cipher = this->jc->o_pages[this->jc->tasks[this->currentTask]->pageSlot] + this->jc->tasks[this->currentTask]->localChunk * *this->jc->CHUNK_SIZE;
    // char* cipher = this->jc->output + (size_t) (this->jc->tasks[this->currentTask]->localChunk * *this->jc->CHUNK_SIZE);
    char* cipher = this->jc->output + (size_t) (this->currentTask * *this->jc->CHUNK_SIZE);
    time::time_point salt_gen_start = time::now();
    // Calculate the hash
    unsigned char* hash = new unsigned char[MD5_DIGEST_LENGTH];
    MD5(((unsigned char*)chunk), dataSize, hash);
    // unsigned char* hash = new unsigned char[SHA256_DIGEST_LENGTH];
    // SHA256(((unsigned char*)chunk), dataSize, hash);
    // just some renaming purposes;
    unsigned char* salt = new unsigned char[16];
    time::time_point salt_gen_end = time::now();
    std::chrono::duration<double, std::milli> salt_gen_duration = salt_gen_end - salt_gen_start;
    this->jc->tasks[this->currentTask]->execTimeLog.push_back(new string("hash," + to_string(salt_gen_duration.count()) + "," + to_string(salt_gen_start.time_since_epoch().count()) + "," + to_string(salt_gen_end.time_since_epoch().count())));
    time::time_point prev_salt_req_start = time::now();
    // Wait For Previous Salt
    this->jc->saltsIndex->isReady(this->currentTask - 1);
    unsigned char* previousSalt = this->jc->saltsIndex->get(this->currentTask - 1);
    time::time_point prev_salt_req_end = time::now();
    std::chrono::duration<double, std::milli> prev_salt_req_duration = prev_salt_req_end - prev_salt_req_start;
    this->jc->tasks[this->currentTask]->execTimeLog.push_back(new string("prev_salt_req," + to_string(prev_salt_req_duration.count()) + "," + to_string(prev_salt_req_start.time_since_epoch().count()) + "," + to_string(prev_salt_req_end.time_since_epoch().count())));

    // generateSalt
    this->generateSalt(hash, previousSalt, salt);
    delete hash;
    // Save salt
    this->jc->saltsIndex->set(this->currentTask, salt);

    // Encrypt
    time::time_point enc_start = time::now();
    AES_KEY key;
    AES_set_encrypt_key(this->jc->dataEncryptionKey, 256, &key);

    AES_cbc_encrypt((unsigned char*)chunk, (unsigned char*) cipher, dataSize, &key, (unsigned char*)salt, AES_ENCRYPT);
    time::time_point enc_end = time::now();
    std::chrono::duration<double, std::milli> enc_duration = enc_end - enc_start;
    this->jc->tasks[this->currentTask]->execTimeLog.push_back(new string("enc," + to_string(enc_duration.count()) + "," + to_string(enc_start.time_since_epoch().count()) + "," + to_string(enc_end.time_since_epoch().count())));
}

void ChunkHandler::_decrypt()
{
    char salt[16];
    unsigned char* x = this->jc->saltsIndex->get(this->currentTask);
    for(int i = 0; i < 16; i++)
    {
        salt[i] = x[i];
    }

    const int dataSize = *this->jc->CHUNK_SIZE;
    char* cipher = this->jc->i_pages[this->jc->tasks[this->currentTask]->pageSlot] + this->jc->tasks[this->currentTask]->localChunk * *this->jc->CHUNK_SIZE;
    char* chunk = this->jc->output + (size_t) (this->currentTask * *this->jc->CHUNK_SIZE);
    AES_KEY key;
    AES_set_decrypt_key(this->jc->dataEncryptionKey, 256, &key);

    AES_cbc_encrypt((unsigned char*)cipher, (unsigned char*) chunk, dataSize, &key, (unsigned char*) salt, AES_DECRYPT);
}

void ChunkHandler::join()
{
    this->t->join();
}

void ChunkHandler::generateSalt(unsigned char* hash, const unsigned char* previousSalt, unsigned char* salt)
{
    for (int i = 0; i < 16; i++)
    {
        hash[i] = hash[i] ^ previousSalt[i];
    }

    AES_KEY key;
    AES_set_encrypt_key(this->jc->saltEncryptionKey, 256, &key);
    AES_ecb_encrypt(hash, salt, &key, AES_ENCRYPT);
}

ChunkHandler::~ChunkHandler()
{
    delete this->t;
    delete this->threadBarrier;
}
