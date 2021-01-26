#include <iostream>

#include "../common/net/socket/socket.h"

#include <map>
#include "../common/constants/comm-flags.h"
#include "ChunkHandler/ChunkHandler.h"

#include <vector>
#define CHUNK_ID_LENGTH 5

using namespace std;

int getRamUsage();

char* buffer = NULL;

int main(int argc, char** argv)
{
    const char* HOST = getenv("DCBC_MASTER_IP");
    const int WH_PORT = atoi(getenv("DCBC_WORKER_HANDLER_PORT"));
    const int PORT = atoi(getenv("DCBC_MASTER_PORT"));

    // Connect to master on 8080
    cout << "Connecting to: " << HOST << ":" << PORT << endl;
    const string workerId = string(argv[1]);
    Socket socket = Socket();
    Connection* conn = socket.Connect(HOST, PORT);
    cout << "Connected !" << endl;

    // Send the workerId to master
    conn->Send(workerId.c_str(), '0', workerId.length());
    

    while(true)
    {
        int dataSize = conn->Recv(&buffer);

        char* chunkId = new char[CHUNK_ID_LENGTH + 1];
        memcpy(chunkId, buffer+1, CHUNK_ID_LENGTH);
        chunkId[CHUNK_ID_LENGTH] = '\0';
        if(buffer[0] == ENCRYPT_CHUNK_FLAG)
        {
            // Pass Worker Handler's Connection to ChunkHandler
            ChunkHandler* chunkHandler = new ChunkHandler(HOST, WH_PORT);
            dataSize = dataSize - (CHUNK_ID_LENGTH + 1);
            chunkHandler->Encrypt(chunkId);            
        }
        else if(buffer[0] == DECRYPT_CHUNK_FLAG)
        {
            // Pass Worker Handler's Connection to ChunkHandler
            ChunkHandler* chunkHandler = new ChunkHandler(HOST, WH_PORT);
            dataSize = dataSize - (CHUNK_ID_LENGTH + 1);
            chunkHandler->Decrypt(buffer, dataSize, chunkId);            
        }
        else
        {
            break;
        }
    }

    return 0;
}

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

int getRamUsage(){ //Note: this value is in KB!
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}
