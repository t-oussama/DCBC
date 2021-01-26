#include "JobManager.h"
#include "../../common/net/socket/socket.h"
#include <string>
#include <fstream>
#include <iostream>
#include <math.h>
#include "../JobContext/JobContext.h"
#include "../WorkerManager/WorkerManager.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mutex>

#define PORT 8080
#define MAX_WORKERS 100
#define TASKS_SOCK_PORT 5050

using namespace std;

JobManager::JobManager(const int min_workers, int pageSize)
{
    this->min_workers = min_workers;
    this->connectionSocket = Socket();
    this->connectionSocket.Bind(PORT);
    this->connectionSocket.Listen(MAX_WORKERS);
    this->workers = new std::vector<WorkerManager*>();
    this->workersIds = std::map<std::string, int>();

    this->tasksSocket = Socket();
    this->tasksSocket.Bind(TASKS_SOCK_PORT);
    this->tasksSocket.Listen(MAX_WORKERS);

    this->HandleConnection();
    this->pageSize = pageSize;
}

int JobManager::ClusterSize()
{
    return this->workers->size();
}

void JobManager::HandleConnection()
{
    char* buffer;
    // while(true)
    while( this->workers->size() < this->min_workers)
    {
        Connection* conn = this->connectionSocket.Accept();
        const int workersIndex = this->workers->size();
        // Recieve the workerId

        const int idSize = conn->Recv(&buffer);
        string id = string(buffer + 1, idSize);
        delete buffer;

        this->workers->push_back(new WorkerManager(&this->tasksSocket, &this->tasksSocketMutex, conn, workersIndex, id));
        this->workersIds.insert({std::string(this->workers->at(workersIndex)->id), workersIndex});
        std::cout << "[8080] Node #" << workersIndex << "connected !" << std::endl;

        this->workers->at(workersIndex)->SetNextWorkerManager(this->workers->at(0));
        if(workersIndex > 0)
        {
            this->workers->at(workersIndex - 1)->SetNextWorkerManager(this->workers->at(workersIndex));
        }
    }
}

void JobManager::Encrypt(const string filePath, const string outputDir, const size_t CHUNK_SIZE)
{
    char* base;
    struct stat s;
    int dataFile = open(filePath.c_str(), O_RDONLY);
    if (fstat(dataFile, &s) < 0)
    {
        cout << "File " << filePath << " not found !" << endl;
    }

    size_t fileSize = s.st_size;
    cout << "File Size: " << fileSize << endl;
    base = (char*) mmap(NULL, this->pageSize * CHUNK_SIZE, PROT_READ, MAP_PRIVATE, dataFile, 0);
    unique_lock<mutex> lck(allDoneMutex);
    unique_lock<mutex> plck(pageDispatchedMutex);
    this->progress = 0;
    const string fileId = "abcd";
    this->chunkCount = ceil(fileSize / CHUNK_SIZE);
    this->saltsIndex = new SaltsIndex(this->chunkCount);
    string workerChunks = "";

    // Setup the worker handler context variables
    JobContext* jc = new JobContext();
    jc->allDone = &this->allDone;
    jc->pageSize = &this->pageSize;
    jc->pageDispatched = &this->pageDispatched;
    jc->pageProgress = &this->pageProgress;
    jc->CHUNK_SIZE = &CHUNK_SIZE;
    jc->chunkCount = &this->chunkCount;
    jc->chunkCountMutex = &this->chunkCountMutex;
    jc->fullClearText = &base;
    jc->fullClearTextSize = &fileSize;
    jc->outputDir = &outputDir;
    jc->progress = &this->progress;
    jc->saltsIndex = this->saltsIndex;
    jc->fileId = &fileId;
    jc->pageCountMutex = &this->pageCountMutex;
    this->dispatchJobContext(jc);

    // Start the encryption process
    for (int i = 0; i < this->chunkCount; i++)
    {
        this->pageProgress = 0;
        if( i > 0 && i % this->pageSize == 0)
        {
            this->pageDispatched.wait(plck, [this] { return this->pageProgress == this->pageSize; });
            munmap(base, this->pageSize * CHUNK_SIZE);
            base = (char*) mmap(NULL, this->pageSize * CHUNK_SIZE, PROT_READ, MAP_PRIVATE, dataFile, i * CHUNK_SIZE);
        }

        const int connection_rrc = i % this->workers->size();
        this->saltsIndex->add(i);
        this->workers->at(connection_rrc)->EncryptChunk(i);
        workerChunks += string(this->workers->at(connection_rrc)->id) + '\n';
    }
    // Wait for & unmap the final page
    this->pageDispatched.wait(plck, [this] { return this->pageProgress == this->pageSize; });
    munmap(base, this->pageSize * CHUNK_SIZE);

    ofstream workerChunksFile = ofstream(outputDir + '/' + fileId + ".maps");
    workerChunksFile << workerChunks;
    workerChunksFile.close();

    this->allDone.wait(lck, [this] { return this->chunkCount == this->progress; });
    // Persist all salts to disk
    ofstream saltsFile = ofstream(outputDir + "/" + fileId + ".salts", ios::binary);
    char* chunkCountString = new char[4];
    for (int i = 0; i < 4; i++)
    {
        chunkCountString[3 - i] = (this->chunkCount >> (i * 8));
    }
    saltsFile.write(chunkCountString, 4);
    delete chunkCountString;

    // Persiting used salts to .salts file
    for (int i = 0; i < this->chunkCount; i++)
    {
        char* salt = this->saltsIndex->get(i);
        if(salt == NULL)
        {
            cout << "Got a NULL for chunk #" << i+2 << endl;
        }
        else
        {
            saltsFile.write(salt, 16);
        }
    }
    saltsFile.close();
    close(dataFile);

}

void JobManager::Decrypt(string filePath, const string outputDir, const size_t CHUNK_SIZE)
{
    unique_lock<mutex> lck(allDoneMutex);
    this->progress = 0;
    const string fileId = "abcd";

    ifstream saltsFile = ifstream(outputDir + "/" + fileId + ".salts", ios::binary);
    saltsFile.seekg(ios::beg);
    if(!saltsFile.is_open())
    {
        cout << "ERROR: Could not open salts file: " << outputDir + "/" + fileId + ".salts" << endl;
    }
    char* sizeString = new char[4];
    saltsFile.read(sizeString, 4);
    this->chunkCount = 0;
    for(int i = 0; i < 3; i++)
    {
        this->chunkCount |= sizeString[3 - i] << (i * 8);
    }

    // this->fullClearText = new char[(long) CHUNK_SIZE * this->chunkCount];

    char* salt = new char[16];
    saltsFile.seekg(4);

    // Setup the worker handler context variables
    JobContext* jc = new JobContext();
    jc->allDone = &this->allDone;
    jc->CHUNK_SIZE = &CHUNK_SIZE;
    jc->chunkCount = &this->chunkCount;
    jc->chunkCountMutex = &this->chunkCountMutex;
    jc->outputDir = &outputDir;
    jc->progress = &this->progress;
    jc->saltsIndex = this->saltsIndex;
    jc->fileId = &fileId;
    ofstream clearTextOutput = ofstream(outputDir + '/' + fileId + ".clear");
    mutex clearTextOutputMutex;
    jc->outputFile = &clearTextOutput;
    jc->outputFileMutex = &clearTextOutputMutex;
    this->dispatchJobContext(jc);

    ifstream workerChunksFile = ifstream(outputDir + '/' + fileId + ".maps");
    string workerId;
    for (int i = 0; i < this->chunkCount; i++)
    {
        saltsFile.read(salt, 16);
        getline(workerChunksFile, workerId);
        this->workers->at(this->workersIds.find(workerId)->second)->DecryptChunk(i, salt);
    }
    workerChunksFile.close();
    delete sizeString;
    delete salt;
    this->allDone.wait(lck, [this] { return this->chunkCount == this->progress; });

    // clearTextOutput.write(this->fullClearText, this->fullClearTextSize);
    clearTextOutput.close();
    delete this->fullClearText;
    cout << "Final Output at: " << outputDir + '/' + fileId + ".clear" << endl;
}

void JobManager::dispatchJobContext(JobContext* jc)
{
    for(int i = 0; i < this->workers->size(); i++)
    {
        this->workers->at(i)->SetJobContext(jc);
    }
}

JobManager::~JobManager()
{
    delete this->connectionHandler;
}