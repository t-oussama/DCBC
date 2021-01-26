#include "./dcbc.h"
#include <iostream>
#include "fcntl.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>
#include <fstream>
#include <vector>
#include "../ChunkHandler/ChunkHandler.h"
#include "../TaskContext/TaskContext.h"
#include "../JobContext/JobContext.h"

#include "openssl/sha.h"

using namespace std;

size_t dcbc(string& filePath, string& outputDir, string& fileId, char* op, const size_t& CHUNK_SIZE, const int& POOL_SIZE, const int& PAGE_SIZE, const int& MAX_PAGES, char** output)
{
    // Open File
    int file = open(filePath.c_str(), O_RDONLY);

    // Calculate file Size
    struct stat s;
    if (fstat(file, &s) < 0)
    {
        cout << "File " << filePath << " not found !" << endl;
    }
    size_t fileSize = s.st_size;
    cout << "Encrypting file of size: " << fileSize << endl;

    // Calculate number of chunks needed
    unsigned int chunkCount = ceil(fileSize / CHUNK_SIZE);
    SaltsIndex saltsIndex(chunkCount);
    if(op[0] == 'D')
    {
        loadSalts(outputDir + "/" + fileId + ".salts", chunkCount, saltsIndex);
    }
    // Init Chunk Handler Pool
    ChunkHandler* chPool[POOL_SIZE];
    vector<unsigned int> availableChunkHandlers;
    mutex availableChunkHandlersRW;

    TaskContext* tc[chunkCount];
    mutex tasksRW;
    unsigned int pendingTask = 0;
    mutex pendingTaskRW;

    // Load Initial Pages
    unsigned int filePages = chunkCount / PAGE_SIZE;
    int effectivePages = min(filePages, (unsigned int) MAX_PAGES);
    char* i_pages[effectivePages];
    // char* o_pages[effectivePages];
    int pagesProgress = effectivePages;
    mutex pagesProgressRW;
    // unsigned int loadedTasks = 0;
    mutex pendingChunksIndexRW;
    int chCounter = 0;

    JobContext jc;
    jc.i_pages = i_pages;
    // jc.o_pages = o_pages;
    *output = new char[fileSize + chunkCount];
    jc.output = *output;
    jc.tasks = tc;
    jc.i_file = file;
    // jc.o_file = outFd;
    jc.totalPages = filePages;
    jc.pagesProgress = &pagesProgress;
    jc.pagesProgressRW = &pagesProgressRW;
    jc.chunkCount = chunkCount;
    jc.loadedTasks = &chCounter;
    jc.chPool = chPool;
    jc.tasksRW = &tasksRW;
    jc.pendingTask = &pendingTask;
    jc.pendingTaskRW = &pendingTaskRW;
    jc.availableChunkHandlers = &availableChunkHandlers;
    jc.availableChunkHandlersRW = &availableChunkHandlersRW;
    jc.saltsIndex = &saltsIndex;
    jc.outputDir = outputDir;
    jc.CHUNK_SIZE = &CHUNK_SIZE;
    jc.PAGE_SIZE = &PAGE_SIZE;
    jc.dataEncryptionKey = (unsigned char*) "01234567890123456789012345678901";
    jc.saltEncryptionKey = new unsigned char[SHA256_DIGEST_LENGTH];
    SHA256(jc.dataEncryptionKey, 32, jc.saltEncryptionKey);


    for (int i = 0; i < POOL_SIZE; i++)
    {
        chPool[i] = new ChunkHandler((unsigned int) i, &jc, op[0] == 'E' ? OP_ENC : OP_DEC);
    }



    for (int i = 0; i < effectivePages; i++)
    {
        // Don't always read i * page_size * chunk size !! check remaining data first
        // Same precaution for remainingPageChunks, it can't be always PAGE_SIZE
        i_pages[i] = (char*) mmap(NULL, (size_t) PAGE_SIZE * CHUNK_SIZE, PROT_READ, MAP_PRIVATE, file, i * PAGE_SIZE * CHUNK_SIZE);
        if(i_pages[i] == MAP_FAILED)
        {
            cerr << "Map failed for page: " << i << endl;
            exit(1);
        }
        // o_pages[i] = (char*) mmap(NULL, PAGE_SIZE * CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, outFd, i * PAGE_SIZE * CHUNK_SIZE);
        int* remainingPageChunks = new int(PAGE_SIZE);
        mutex* remainingPageChunksRW = new mutex();

        for (int j = 0; j < PAGE_SIZE; j++)
        {
            tc[chCounter] = new TaskContext();
            tc[chCounter]->pageSlot = i;
            tc[chCounter]->localChunk = j;
            tc[chCounter]->remainingPageChunks = remainingPageChunks;
            tc[chCounter]->remainingPageChunksRW = remainingPageChunksRW;
            tc[chCounter]->pageSize = PAGE_SIZE;
            chCounter ++;
        }
    }

    pendingTask = POOL_SIZE;
    for (int i = 0; i < POOL_SIZE; i++)
    {
        chPool[i]->runTask(i);
    }
    cout << "All running" << endl;
    
    for (int i = 0; i < POOL_SIZE; i++)
    {
        chPool[i]->join();
        delete chPool[i];
    }

    close(file);

    // Save salts to file
    if(op[0] == 'E')
    {
        saveSalts(outputDir + "/" + fileId + ".salts", chunkCount, saltsIndex);

        // // Log the timers
        // for(int i = 0; i < chunkCount; i++)
        // {
        //     ofstream chunkLog("./logs/chunk" + to_string(i) + ".log", ios::app);
        //     for(int j = 0; j < tc[i]->execTimeLog.size(); j++)
        //     {
        //         chunkLog << *(tc[i]->execTimeLog[j]) << '\n';
        //     }
        //     chunkLog.close();
        //     delete tc[i];
        // }
    }

    return fileSize;
}

void saveSalts(string output, unsigned int chunkCount, SaltsIndex& saltsIndex)
{
    ofstream saltsFile = ofstream(output, ios::binary);
    char* chunkCountString = new char[4];
    for (int i = 0; i < 4; i++)
    {
        chunkCountString[3 - i] = (chunkCount >> (i * 8));
    }
    saltsFile.write(chunkCountString, 4);
    delete chunkCountString;

    for (int i = 0; i < chunkCount; i++)
    {
        char* salt = (char*) saltsIndex.get(i);
        if(salt == NULL)
        {
            cerr << "Got a NULL for chunk #" << i+2 << endl;
        }
        else
        {
            saltsFile.write(salt, 16);
        }
    }
    saltsFile.close();
}

void loadSalts(string output, unsigned int& chunkCount, SaltsIndex& saltsIndex)
{
    ifstream saltsFile = ifstream(output, ios::binary);
    saltsFile.seekg(ios::beg);
    if(!saltsFile.is_open())
    {
        cerr << "ERROR: Could not open salts file: " << output << endl;
    }
    char* sizeString = new char[4];
    saltsFile.read(sizeString, 4);
    chunkCount = 0;
    for(int i = 0; i < 3; i++)
    {
        chunkCount |= sizeString[3 - i] << (i * 8);
    }
    delete sizeString;

    // this->fullClearText = new char[(long) CHUNK_SIZE * this->chunkCount];

    saltsFile.seekg(4);
    char* salt;
    for (int i = 0; i < chunkCount; i++)
    {
        salt = new char[16];
        saltsFile.read(salt, 16);
        saltsIndex.set(i, (unsigned char*) salt);
    }
}