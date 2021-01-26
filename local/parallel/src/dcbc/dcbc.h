#include "../SaltsIndex/SaltsIndex.h"

void saveSalts(string output, unsigned int chunkCount, SaltsIndex& saltsIndex);
void loadSalts(string output, unsigned int& chunkCount, SaltsIndex& saltsIndex);
void persistOutput(string path, char* jc, size_t& fileSize);
size_t dcbc(string& filePath, string& outputDir, string& fileId, char* op, const size_t& CHUNK_SIZE, const int& POOL_SIZE, const int& PAGE_SIZE, const int& MAX_PAGES, char** output);