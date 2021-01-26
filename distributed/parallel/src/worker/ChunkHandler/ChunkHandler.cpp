#include "ChunkHandler.h";
#include "../../common/constants/comm-flags.h"
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>

#include "../../common/net/socket/socket.h"

// Crypto++ includes
#include "openssl/sha.h"
#include "openssl/aes.h"


// Just for logging
#include <chrono>

#define CHUNK_ID_LENGTH 5
#define time std::chrono::high_resolution_clock

using namespace std;

ChunkHandler::ChunkHandler(const char* host, const int port)
{
    this->HOST = host;
    this->PORT = port;
    this->input = NULL;
    // this->threadBarrier.lock();
}

void ChunkHandler::Encrypt(char* chunkId)
{
    this->t = new thread(&ChunkHandler::encrypt, this, chunkId);
}

void ChunkHandler::Decrypt(char* buffer, const int dataSize, char* chunkId)
{
    this->t = new thread(&ChunkHandler::decrypt, this, buffer, chunkId);
}

// void ChunkHandler::setBuffer(char* buffer, char* chunkId)
// {
//     // Set the message to queue
//     this->input = buffer;
//     this->threadBarrier.unlock();
// }

void ChunkHandler::connectToWorkerHandler()
{
    // Connect to WorkerHandler on 5050
    char* buffer;
    Socket whSocket = Socket();
    cout << "Connecting to: " << this->HOST << ":" << this->PORT << endl;
    this->conn = whSocket.Connect(this->HOST, this->PORT);
    // Wait for the outputDir
    const int outputDirSize = this->conn->Recv(&buffer);
    this->outputDir = string(buffer+1, outputDirSize-1);
    system(("mkdir -p " + this->outputDir + "/ciphers").c_str());
    delete buffer;
    // this->workerId = workerId;
}

void ChunkHandler::encrypt(char* chunkId)
{
    // system("mkdir -p ./logs");
    // ofstream chunkLog = ofstream("./logs/chunk" + to_string((int) chunkId[CHUNK_ID_LENGTH - 1]), ios::app);
    // Log Enc started for chunkId
    // time::time_point encStart = time::now();

    char* buffer;

    // time::time_point connStart = time::now();
    this->connectToWorkerHandler();
    // time::time_point connEnd = time::now();
    // std::chrono::duration<double, std::milli> connDuration = connEnd - connStart;
    // chunkLog << "conn," << connDuration.count() << "," << connStart.time_since_epoch().count() << "," << connEnd.time_since_epoch().count() << '\n';


    // time::time_point chunk_req_start = time::now();
    this->conn->Send("", GET_CHUNK_FLAG, 0);
    // time::time_point chunk_req_end = time::now();
    // std::chrono::duration<double, std::milli> chunk_req_duration = chunk_req_end - chunk_req_start;
    // chunkLog << "chunk_req," << chunk_req_duration.count() << "," << chunk_req_start.time_since_epoch().count() << "," << chunk_req_end.time_since_epoch().count() << '\n';

    // time::time_point chunk_resp_start = time::now();
    const int dataSize = this->conn->Recv(&buffer);
    // time::time_point chunk_resp_end = time::now();
    // std::chrono::duration<double, std::milli> chunk_resp_duration = chunk_resp_end - chunk_resp_start;
    // chunkLog << "chunk_resp," << chunk_resp_duration.count() << "," << chunk_resp_start.time_since_epoch().count() << "," << chunk_resp_end.time_since_epoch().count() << '\n';

    // time::time_point salt_gen_start = time::now();
    char* chunk = buffer + 1;
    // Calculate the hash
    string chunkIndex = to_string((int)chunkId[CHUNK_ID_LENGTH - 1]);

    unsigned char* hash = new unsigned char[SHA256_DIGEST_LENGTH];
    SHA256(((unsigned char*)chunk), dataSize, hash);
    // just some renaming purposes;
    char* salt = (char*) hash;
    // time::time_point salt_gen_end = time::now();
    // std::chrono::duration<double, std::milli> salt_gen_duration = salt_gen_end - salt_gen_start;
    // chunkLog << "salt_gen," << salt_gen_duration.count() << "," << salt_gen_start.time_since_epoch().count() << "," << salt_gen_end.time_since_epoch().count() << '\n';
    // char* salt = new char[16];
    // memcpy(salt, hash, 16);
    // delete hash;

    // time::time_point prev_salt_req_start = time::now();
    // Send Request For Previous Salt
    this->conn->Send("", GET_PREV_SALT_FLAG, 0);
    // time::time_point prev_salt_req_end = time::now();
    // std::chrono::duration<double, std::milli> prev_salt_req_duration = prev_salt_req_end - prev_salt_req_start;
    // chunkLog << "prev_salt_req," << prev_salt_req_duration.count() << "," << prev_salt_req_start.time_since_epoch().count() << "," << prev_salt_req_end.time_since_epoch().count() << '\n';
    
    // Wait For the response
    char* prevSaltBuffer;
    // time::time_point prev_salt_resp_start = time::now();
    int prevDataSize = this->conn->Recv(&prevSaltBuffer);
    // time::time_point prev_salt_resp_end = time::now();
    // std::chrono::duration<double, std::milli> prev_salt_resp_duration = prev_salt_resp_end - prev_salt_resp_start;
    // chunkLog << "prev_salt_resp," << prev_salt_resp_duration.count() << "," << prev_salt_resp_start.time_since_epoch().count() << "," << prev_salt_resp_end.time_since_epoch().count() << '\n';

    char* previousSalt = prevSaltBuffer + 1;
    // char* previousSalt = new char[17];
    // memcpy(previousSalt, prevSaltBuffer + 1, 16);
    // previousSalt[16] = '\0';
    // delete prevSaltBuffer;

    // Combine
    combine(salt, previousSalt);

    // Save salt in master
    // time::time_point salt_set_start = time::now();
    this->conn->Send(salt, SET_SALT_FLAG, 16);
    // time::time_point salt_set_end = time::now();
    // std::chrono::duration<double, std::milli> salt_set_duration = salt_set_end - salt_set_start;
    // chunkLog << "salt_set," << salt_set_duration.count() << "," << salt_set_start.time_since_epoch().count() << "," << salt_set_end.time_since_epoch().count() << '\n';

    // Encrypt
    // time::time_point enc_start = time::now();
    // TODO: Generate the key randomly
    unsigned char* plainTextKey = (unsigned char*)"01234567890123456789012345678901";
    AES_KEY key;
    AES_set_encrypt_key(plainTextKey, 256, &key);

    unsigned char* ciphertext = new unsigned char[dataSize];
    AES_cbc_encrypt((unsigned char*)chunk, ciphertext, dataSize, &key, (unsigned char*)salt, AES_ENCRYPT);
    // time::time_point enc_end = time::now();
    // std::chrono::duration<double, std::milli> enc_duration = enc_end - enc_start;
    // chunkLog << "enc," << enc_duration.count() << "," << enc_start.time_since_epoch().count() << "," << enc_end.time_since_epoch().count() << '\n';
    // use \0 to limit chunkId to the base id
    chunkId[CHUNK_ID_LENGTH - 1] = '\0';
    // Write cihper text to output
    
    // time::time_point cipher_write_start = time::now();
    ofstream cipherFile = ofstream(this->outputDir + "/ciphers/" + string(chunkId) + chunkIndex + ".cipher", ios::binary);
    cipherFile.write((char*)ciphertext, dataSize);
    cipherFile.close();

    // time::time_point cipher_write_end = time::now();
    // std::chrono::duration<double, std::milli> cipher_write_duration = cipher_write_end - cipher_write_start;
    // chunkLog << "cipher_write," << cipher_write_duration.count() << "," << cipher_write_start.time_since_epoch().count() << "," << cipher_write_end.time_since_epoch().count() << '\n';

    // time::time_point signal_done_start = time::now();
    this->SignalDone();
    // time::time_point signal_done_end = time::now();
    // std::chrono::duration<double, std::milli> signal_done_duration = signal_done_end - signal_done_start;
    // chunkLog << "signal_done," << signal_done_duration.count() << "," << signal_done_start.time_since_epoch().count() << "," << signal_done_end.time_since_epoch().count() << '\n';
    // Clean up
    delete ciphertext;
    // delete previousSalt;
    delete prevSaltBuffer;
    delete salt;
    delete chunkId;
    delete buffer;
    // chunkLog.close();
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

    char* clearText = new char[CIPHER_SIZE];

    unsigned char* plainTextKey = (unsigned char*)"01234567890123456789012345678901";
    AES_KEY key;
    AES_set_decrypt_key(plainTextKey, 256, &key);

    AES_cbc_encrypt((unsigned char*)ciphertext, (unsigned char*)clearText, CIPHER_SIZE, &key, salt, AES_DECRYPT);
    delete ciphertext;

    this->conn->Send(clearText, SET_CHUNK_FLAG, CIPHER_SIZE);
    delete chunkBaseId;
    delete clearText;
    // delete chunkId;
    delete buffer;
}

void ChunkHandler::combine(char* salt, const char* previousSalt)
{
    for (int i = 0; i < 16; i++)
    {
        salt[i] = salt[i] ^ previousSalt[i];
    }
}

// Methods that send messages to the master

void ChunkHandler::RequestPreviousSalt(const char* currentChunkId)
{
    this->conn->Send(currentChunkId, GET_PREV_SALT_FLAG, CHUNK_ID_LENGTH);
}

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
