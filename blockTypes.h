#pragma once

#include <cstddef>
#include <vector>

// Estructura ultracompacta para un registro de rating (12 bytes en total)
struct RatingRecord {
  int userID;   // 4 bytes
  int movieID;  // 4 bytes
  float rating; // 4 bytes
};

// Cabecera del bloque físico de disco: 3 enteros (12 bytes)
constexpr size_t HEADER_SIZE = sizeof(int) * 3;

// Calcula cuántos registros completos caben en un bloque de 'blockSize' bytes
inline size_t CalcMaxRecordsPerBlock(size_t blockSize) {
  if (blockSize <= HEADER_SIZE) {
    return 0;
  }
  size_t payloadBytes = blockSize - HEADER_SIZE;
  return payloadBytes / sizeof(RatingRecord);
}

// Estructura del bloque físico de disco en memoria (tamaño definido en ejecución)
struct DataBlock {
  int blockID;     // Identificador único del bloque (-1 si el slot del buffer está libre)
  int recordCount; // Cuántos registros válidos contiene este bloque
  int nextBlockID; // Puntero lógico al siguiente bloque (-1 si es el último)

  // Registros almacenados en este bloque (capacidad máxima = maxRecordsPerBlock)
  std::vector<RatingRecord> records;
};