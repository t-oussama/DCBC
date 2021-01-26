#include "ChunkHandler.h";
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include "../../common/net/socket/socket.h"
// Crypto++ includes
#include "openssl/sha.h"
#include "openssl/aes.h"

#include "../../common/constants/comm-flags.h"

#define CHUNK_ID_LENGTH 5

using namespace std;

ChunkHandler::ChunkHandler(const char* host, const int port)
{
    this->HOST = host;
    this->PORT = port;
}

void ChunkHandler::Encrypt(char* chunkId)
{
    this->t = new thread(&ChunkHandler::encrypt, this, chunkId);
}

void ChunkHandler::Decrypt(char* buffer, const int dataSize, char* chunkId)
{
    this->t = new thread(&ChunkHandler::decrypt, this, buffer, chunkId);
}

void ChunkHandler::connectToWorkerHandler()
{
    // Connect to WorkerHandler on 5050
    char* buffer;
    Socket whSocket = Socket();
    this->conn = whSocket.Connect(this->HOST, this->PORT);
    // Wait for the outputDir
    const int outputDirSize = this->conn->Recv(&buffer);
    this->outputDir = string(buffer+1, outputDirSize-1);
    system(("mkdir -p " + this->outputDir + "/ciphers").c_str());
    delete buffer;
}

void ChunkHandler::encrypt(char* chunkId)
{
    char* buffer;
    this->connectToWorkerHandler();
    this->conn->Send("", GET_CHUNK_FLAG, 0);
    const int dataSize = this->conn->Recv(&buffer);
    char* ciphertext = buffer + 1;

    string chunkIndex = to_string((int)chunkId[CHUNK_ID_LENGTH - 1]);
    // use \0 to limit chunkId to the base id
    chunkId[CHUNK_ID_LENGTH - 1] = '\0';

    // Write cihper text to output
    ofstream cipherFile = ofstream(this->outputDir + "/ciphers/" + string(chunkId) + chunkIndex + ".cipher", ios::binary);
    cipherFile.write((char*)ciphertext, dataSize);
    cipherFile.close();
    this->SignalDone();
    // Clean up
    delete chunkId;
    delete buffer;
}

void ChunkHandler::decrypt(char* buffer, char* chunkId)
{
    this->connectToWorkerHandler();
    unsigned char* salt = (unsigned char*)(buffer + 1 + CHUNK_ID_LENGTH);
    char* chunkBaseId = chunkId;
    int chunkIndex = (int)buffer[CHUNK_ID_LENGTH];
    ifstream cipherFile = ifstream(this->outputDir + "/ciphers/" + string(chunkBaseId, CHUNK_ID_LENGTH - 1) + to_string(chunkIndex) + ".cipher", ios::binary);
    const long int CIPHER_SIZE = 128 * 1024 * 1024;

    char* ciphertext = new char[CIPHER_SIZE];
    cipherFile.read(ciphertext, CIPHER_SIZE);
    cipherFile.close();

    this->conn->Send(ciphertext, SET_CHUNK_FLAG, CIPHER_SIZE);
    delete ciphertext;
    delete chunkBaseId;
    delete buffer;
}

// Methods that send messages to the master
void ChunkHandler::SignalDone()
{
    char* message = "\0";
    this->conn->Send(message, SIGNAL_DONE_FLAG, 1);
}

void ChunkHandler::SignalResetDone()
{
    char* message = "\0";
    this->conn->Send(message, SIGNAL_RESET_DONE_FLAG, 1);
}
