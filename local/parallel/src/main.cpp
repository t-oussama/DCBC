#include <string>
#include <fstream>
#include <iostream>
#include "./dcbc/dcbc.h"

// Just for logging
#include <chrono>
#define time std::chrono::high_resolution_clock

using namespace std;


int main(int argc, char** argv)
{
    char* op = argv[7];
    bool _persistOutput = false;
    string filePath = string(argv[6]);
    const size_t CHUNK_SIZE = atol(argv[2]);
    const int PAGE_SIZE = atoi(argv[3]);
    const int MAX_PAGES = atoi(argv[4]);
    const int POOL_SIZE = atoi(argv[5]);
    _persistOutput = atoi(argv[8]);

    string outputDir = "./OUTPUT/" + string(argv[2]) + "/" + string(argv[1]);
    system(("mkdir -p " + outputDir).c_str());
    string fileId = "abcd";
    time::time_point op_start = time::now();

    char* output;
    size_t output_size = dcbc(filePath, outputDir, fileId, op, CHUNK_SIZE, POOL_SIZE, PAGE_SIZE, MAX_PAGES, &output);

    if(_persistOutput)
    {
        persistOutput(outputDir + "/" + fileId + (op[0] == 'E' ? ".cipher" : ".clear"), output, output_size);
    }
    delete output;
    // Print op output results
    time::time_point op_end = time::now();
    std::chrono::duration<double, std::milli> op_duration = op_end - op_start;
    cout << "Took: " << op_duration.count() / 1000 << endl;
    ofstream totalsLog("./logs/totals.log", ios::app);
    totalsLog << op[0] << output_size << "," << argv[2] << "," << PAGE_SIZE << "," << MAX_PAGES << "," << POOL_SIZE << "," << op_duration.count() << '\n';
    totalsLog.close();

    return 0;
}

void persistOutput(string path, char* output, size_t& fileSize)
{
    time::time_point w_start = time::now();
    ofstream outstream(path, ios::binary);
    if (! outstream.is_open())
    {
        cerr << "Error opening write file: " << path << endl;
    }
    outstream.write(output, fileSize);
    outstream.close();
    time::time_point w_end = time::now();
    std::chrono::duration<double, std::milli> w_duration = w_end - w_start;
    cout << "Writing Took: " << w_duration.count() / 1000 << endl;
}

