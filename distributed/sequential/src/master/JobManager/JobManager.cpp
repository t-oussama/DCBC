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
#include "openssl/aes.h"

#define PORT 8080
#define MAX_WORKERS 100
#define TASKS_SOCK_PORT 5050

using namespace std;

JobManager::JobManager(const int min_workers)
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
}

int JobManager::ClusterSize()
{
    return this->workers->size();
}

void JobManager::HandleConnection()
{
    char* buffer;
    while( this->workers->size() < this->min_workers)
    {
        Connection* conn = this->connectionSocket.Accept();
        const int workersIndex = this->workers->size();
        // Recieve the workerId

        const int idSize = conn->Recv(&buffer);
        string id = string(buffer + 1, idSize);
        delete buffer;

        this->workers->push_back(new WorkerManager(&this->tasksSocket, &this->tasksSocketMutex, conn, id));
        this->workersIds.insert({std::string(this->workers->at(workersIndex)->id), workersIndex});
        std::cout << "[8080] Node #" << workersIndex << "connected !" << std::endl;
    }
}

void JobManager::Encrypt(const string filePath, const string outputDir, const size_t CHUNK_SIZE)
{

    ifstream dataFile(filePath, ifstream::ate);
    size_t fileSize = dataFile.tellg();
    char* base = new char[fileSize];
    dataFile.seekg(0, dataFile.beg);
    dataFile.read(base, fileSize);
    
    cout << "File Size: " << fileSize << endl;
    unique_lock<mutex> lck(this->allDoneMutex);
    this->progress = 0;
    const string fileId = "abcd";
    this->chunkCount = ceil(fileSize / CHUNK_SIZE);
    string workerChunks = "";
    // Encrypt
    // TODO: Generate the key randomly
    unsigned char* plainTextKey = (unsigned char*)"01234567890123456789012345678901";
    AES_KEY key;
    AES_set_encrypt_key(plainTextKey, 256, &key);

    unsigned char* ciphertext = new unsigned char[fileSize + 1];
    char iv[17] = "1234567890123456";
    cout << "encrypting ...." << endl;
    AES_cbc_encrypt((unsigned char*)base, ciphertext, fileSize, &key, (unsigned char*)iv, AES_ENCRYPT);
    cout << "Encrypted !" << endl;
    // Setup the worker handler context variables
    JobContext* jc = new JobContext();
    jc->allDone = &this->allDone;
    jc->CHUNK_SIZE = &CHUNK_SIZE;
    jc->chunkCount = &this->chunkCount;
    jc->chunkCountMutex = &this->chunkCountMutex;
    jc->fullClearText = (char*) ciphertext;
    jc->fullClearTextSize = &fileSize;
    jc->outputDir = &outputDir;
    jc->progress = &this->progress;
    jc->fileId = &fileId;
    this->dispatchJobContext(jc);

    // Send result for persistence
    for (int i = 0; i < this->chunkCount; i++)
    {
        cout << "Sending chunk #" << i << endl;
        const int connection_rrc = i % this->workers->size();
        this->workers->at(connection_rrc)->SaveCipher(i);
        workerChunks += string(this->workers->at(connection_rrc)->id) + '\n';
    }

    ofstream workerChunksFile = ofstream(outputDir + '/' + fileId + ".maps");
    workerChunksFile << workerChunks;
    workerChunksFile.close();

    this->allDone.wait(lck, [this] { return this->chunkCount == this->progress; });

    munmap(base, fileSize);
    delete ciphertext;
    dataFile.close();

}

void JobManager::Decrypt(string filePath, const string outputDir, const size_t CHUNK_SIZE)
{
    unique_lock<mutex> lck(allDoneMutex);
    this->progress = 0;
    const string fileId = "abcd";

    // Setup the worker handler context variables
    this->chunkCount = 16;
    this->fullClearText = new char[(long) 2*1024*1024*1024];
    JobContext* jc = new JobContext();
    jc->allDone = &this->allDone;
    jc->CHUNK_SIZE = &CHUNK_SIZE;
    jc->chunkCount = &this->chunkCount;
    jc->chunkCountMutex = &this->chunkCountMutex;
    jc->outputDir = &outputDir;
    jc->progress = &this->progress;
    jc->fileId = &fileId;
    jc->fullClearText = this->fullClearText;
    jc->fullClearTextSize = &this->fullClearTextSize;
    this->dispatchJobContext(jc);

    ifstream workerChunksFile = ifstream(outputDir + '/' + fileId + ".maps");
    string workerId;
    for (int i = 0; i < this->chunkCount; i++)
    {
        getline(workerChunksFile, workerId);
        this->workers->at(this->workersIds.find(workerId)->second)->GetChunk(i);
    }
    workerChunksFile.close();

    this->allDone.wait(lck, [this] { return this->chunkCount == this->progress; });

    char* clearText = new char[this->fullClearTextSize];

    char iv[17] = "1234567890123456";

    unsigned char* plainTextKey = (unsigned char*)"01234567890123456789012345678901";
    AES_KEY key;
    AES_set_decrypt_key(plainTextKey, 256, &key);
    AES_cbc_encrypt((unsigned char*) this->fullClearText, (unsigned char*)clearText, this->fullClearTextSize, &key, (unsigned char*)iv, AES_DECRYPT);

    ofstream clearTextOutput = ofstream(outputDir + '/' + fileId + ".clear");
    clearTextOutput.write(clearText, this->fullClearTextSize);
    clearTextOutput.close();
    delete this->fullClearText;
    delete clearText;
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