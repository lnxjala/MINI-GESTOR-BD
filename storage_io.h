#pragma once

#include "blockTypes.h"
#include <cstddef>
#include <fstream>
#include <string>

// Escribe un bloque de 'blockSize' bytes en su offset exacto dentro del archivo
// binario
bool writeBlock(std::ofstream &outFile, const DataBlock &block,
                size_t blockSize);

// Lee un bloque específico de 'blockSize' bytes desde el archivo binario hacia
// la RAM
bool readBlock(std::ifstream &inFile, DataBlock &outBlock, int blockID,
               size_t blockSize, size_t maxRecordsPerBlock);

// Convierte secuencialmente un archivo .tsv a un archivo binario paginado en
// bloques de 'blockSize' bytes
bool ConvertTsvToBin(const std::string &tsvPath, const std::string &binPath,
                     size_t blockSize, size_t maxRecordsPerBlock,
                     int &outTotalBlocks, size_t &outTotalRecords);
