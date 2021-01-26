#include <iostream>
#include <string>
#include <fstream>
#include "fcntl.h"
#include <sys/stat.h>
#include <sys/mman.h>

// Crypto++ includes
#include "openssl/sha.h"
#include "openssl/aes.h"

// Just for logging
#include <chrono>
#define time std::chrono::high_resolution_clock

using namespace std;

int main(int argc, char** argv)
{
    string filePath = string(argv[2]);
    string outputDir = "./OUTPUT/" + string(argv[1]);
    system(("mkdir -p " + outputDir).c_str());
    time::time_point op_start = time::now();
    int file = open(filePath.c_str(), O_RDONLY);
    // Calculate file Size
    struct stat s;
    if (fstat(file, &s) < 0)
    {
        cout << "File " << filePath << " not found !" << endl;
    }
    size_t fileSize = s.st_size;

    char* data = (char*) mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, file, 0);
    // TODO: Generate the key randomly
    unsigned char* plainTextKey = (unsigned char*)"01234567890123456789012345678901";
    AES_KEY key;
    AES_set_encrypt_key(plainTextKey, 256, &key);
    unsigned char* ciphertext = new unsigned char[fileSize + 1];
    char iv[17] = "1234567890123456";
    AES_cbc_encrypt((unsigned char*)data, ciphertext, fileSize, &key, (unsigned char*)iv, AES_ENCRYPT);
    // time::time_point enc_end = time::now();
    // std::chrono::duration<double, std::milli> enc_duration = enc_end - enc_start;
    // chunkLog << "enc," << enc_duration.count() << "," << enc_start.time_since_epoch().count() << "," << enc_end.time_since_epoch().count() << '\n';
    
    // Write cihper text to output
    time::time_point w_start = time::now();
    ofstream cipherFile = ofstream(outputDir + "/abcd.cipher", ios::binary);
    cipherFile.write((char*)ciphertext, fileSize);
    cipherFile.close();
    time::time_point w_end = time::now();
    std::chrono::duration<double, std::milli> w_duration = w_end - w_start;
    cout << "Writing Took: " << w_duration.count() / 1000 << endl;

    delete ciphertext;
    time::time_point op_end = time::now();
    std::chrono::duration<double, std::milli> op_duration = op_end - op_start;
    cout << "Took: " << op_duration.count() / 1000 << endl;
    ofstream logFile = ofstream("./logs/totals.log", ios::app);
    logFile << op_duration.count() << endl;
    logFile.close();
    return 0;
}
