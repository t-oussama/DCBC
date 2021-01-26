#include <iostream>
#include <fstream>
#include <ctime>
#include <math.h>
#define OneMB 1048576

#include "../common/net/socket/socket.h"

#include <thread>

#include <mutex>
#include <map>
#include "../common/constants/comm-flags.h"
#include <cstring>
#include <cstdlib>

#include "./JobManager/JobManager.h"

#define CHUNK_ID_LENGTH 5

using namespace std;

int getRamUsage();

string outputDir;
int CHUNK_SIZE;

int main(int argc, char **argv)
{
    const string fileName = string(argv[1]) + "GB_data";
    CHUNK_SIZE = atoi(argv[2]) * OneMB;
    char op = argv[4][0];
    int min_workers = atoi(argv[3]);

    const string dirPath = string(getenv("DCBC_DATA"));

    JobManager* jobManager = new JobManager(min_workers);
    // char op;
    // cin >> &op;
    const int clusterSize = jobManager->ClusterSize();
    outputDir = "./OUTPUT/" + string(argv[2]) + "/" + to_string(clusterSize) + "/" + string(argv[1]);

    system("mkdir -p ./logs");
    const long double opStart = time(0);
    cout << "wtf..." << endl;
    if (op == 'E')
    {
        cout << "encrypting..." << endl;
        jobManager->Encrypt(dirPath + fileName, outputDir, CHUNK_SIZE);
    }
    else if (op == 'D')
    {
        cout << "decrypting..." << endl;
        jobManager->Decrypt(dirPath + fileName, outputDir, CHUNK_SIZE);
    }
    const long double opEnd = time(0);

    cout << "All done !" << endl;
    cout << "Took: " << opEnd - opStart << endl;

    ofstream logFile;
    logFile.open("./logs/master.log", ios::app);
    // Operation, file size, duration, ram, workers count, chunk size, iteration
    if (op == 'E')
    {
        logFile << "ENC," << string(argv[1]) << "," << opEnd - opStart << "," << (getRamUsage() / 1024) << "," << clusterSize << "," << CHUNK_SIZE / (1024 * 1024) << endl;
    }
    else
    {
        logFile << "DEC," << string(argv[1]) << "," << opEnd - opStart << "," << (getRamUsage() / 1024) << "," << clusterSize << "," << CHUNK_SIZE / (1024 * 1024) << endl;
    }
    logFile.close();

    // Workers Reset
    // JobManager->ResetAll();

    delete jobManager;
    return 0;
}

int parseLine(char *line)
{
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char *p = line;
    while (*p < '0' || *p > '9')
        p++;
    line[i - 3] = '\0';
    i = atoi(p);
    return i;
}

//Note: this value is in KB!
int getRamUsage()
{
    FILE *file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}
