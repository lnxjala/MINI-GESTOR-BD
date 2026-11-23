#pragma once

#include "blockTypes.h"
#include <cstddef>
#include <fstream>

// Capacidad máxima de bloques que pueden estar cargados en RAM al mismo tiempo
const int BUFFER_CAPACITY = 6;

// Inicializa todos los slots del buffer como vacíos (blockID = -1)
void InitBuffer(size_t maxRecordsPerBlock);

// Muestra el resumen actual del buffer: tamaño de bloque, bloques en disco,
// cargados y libres
void ShowBufferStatus(int totalDiskBlocks, size_t blockSize,
                      size_t maxRecordsPerBlock);

// Carga un bloque específico desde el archivo binario a un slot libre del
// buffer
bool LoadBlockToBuffer(std::ifstream &binFile, int targetBlockID,
                       int totalDiskBlocks, size_t blockSize,
                       size_t maxRecordsPerBlock);

// Quita un bloque del buffer y libera su espacio para cargar otro
bool RemoveBlockFromBuffer(int targetBlockID);

// Muestra los registros válidos de un bloque que ya esté cargado en el buffer
void ShowLoadedBlockRecords(int targetBlockID);
