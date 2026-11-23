#include "bufferPool.h"
#include "storage_io.h"
#include <iomanip>
#include <iostream>
#include <vector>

// Arreglo en memoria que simula el Buffer Pool de capacidad fija
static std::vector<DataBlock> BUFFER_POOL;

// Cuenta cuántos slots del buffer tienen un bloque válido cargado
static int CountLoadedBlocks() {
  int count = 0;
  for (const auto &blk : BUFFER_POOL) {
    if (blk.blockID != -1) {
      count++;
    }
  }
  return count;
}

// Busca si un blockID ya está en el buffer; devuelve su índice o -1 si no está
static int FindBlockInBuffer(int targetBlockID) {
  for (int i = 0; i < static_cast<int>(BUFFER_POOL.size()); i++) {
    if (BUFFER_POOL[i].blockID == targetBlockID) {
      return i;
    }
  }
  return -1;
}

// Busca el primer slot libre (blockID == -1); devuelve su índice o -1 si está
// lleno
static int FindFreeSlot() {
  for (int i = 0; i < static_cast<int>(BUFFER_POOL.size()); i++) {
    if (BUFFER_POOL[i].blockID == -1) {
      return i;
    }
  }
  return -1;
}

void InitBuffer(size_t maxRecordsPerBlock) {
  BUFFER_POOL.assign(BUFFER_CAPACITY, DataBlock{});
  for (auto &blk : BUFFER_POOL) {
    blk.blockID = -1; // -1 indica que el slot está disponible
    blk.recordCount = 0;
    blk.nextBlockID = -1;
    blk.records.assign(maxRecordsPerBlock, RatingRecord{0, 0, 0.0f});
  }
}

void ShowBufferStatus(int totalDiskBlocks, size_t blockSize,
                      size_t maxRecordsPerBlock) {
  int loaded = CountLoadedBlocks();
  int available = BUFFER_CAPACITY - loaded;

  std::cout << "\n\n\n--- ESTADO DEL BUFFER POOL ---\n";
  std::cout << "Tamaño de bloque         : " << blockSize << " bytes ("
            << maxRecordsPerBlock << " registros máx.)\n";
  std::cout << "Bloques creados en disco : " << totalDiskBlocks << "\n";
  std::cout << "Capacidad del buffer     : " << BUFFER_CAPACITY << " bloques\n";
  std::cout << "Bloques cargados         : " << loaded << "\n";
  std::cout << "IDs cargados             : ";
  bool any = false;
  for (const auto &blk : BUFFER_POOL) {
    if (blk.blockID != -1) {
      std::cout << blk.blockID << " ";
      any = true;
    }
  }
  if (!any) {
    std::cout << "(ninguno)";
  }
  std::cout << "\n";
  std::cout << "Cantidad disponible      : " << available << "\n";
  std::cout << "------------------------------\n";
}

bool LoadBlockToBuffer(std::ifstream &binFile, int targetBlockID,
                       int totalDiskBlocks, size_t blockSize,
                       size_t maxRecordsPerBlock) {
  // Validar que el ID solicitado exista dentro del archivo binario
  if (targetBlockID < 0 || targetBlockID >= totalDiskBlocks) {
    std::cout << "Ese blockID no existe. Rango válido: 0 a "
              << (totalDiskBlocks > 0 ? totalDiskBlocks - 1 : 0) << ".\n";
    return false;
  }

  // Evitar cargarlo dos veces si ya está presente en el buffer
  int existingIdx = FindBlockInBuffer(targetBlockID);
  if (existingIdx != -1) {
    std::cout << "El bloque " << targetBlockID << " ya está cargado en el slot "
              << existingIdx << ".\n";
    return true;
  }

  // Verificar si queda espacio libre en el buffer
  int freeSlot = FindFreeSlot();
  if (freeSlot == -1) {
    std::cout << "Buffer lleno: no hay espacio disponible (" << BUFFER_CAPACITY
              << "/" << BUFFER_CAPACITY << "). Quita un bloque primero.\n";
    return false;
  }

  // Leer únicamente los 'blockSize' bytes correspondientes a targetBlockID
  DataBlock tempBlock;
  if (!readBlock(binFile, tempBlock, targetBlockID, blockSize,
                 maxRecordsPerBlock)) {
    std::cout << "No se pudo leer el bloque " << targetBlockID
              << " desde el archivo binario.\n";
    return false;
  }

  // Colocar el bloque leído en el slot libre encontrado
  BUFFER_POOL[static_cast<size_t>(freeSlot)] = tempBlock;
  std::cout << "Bloque " << targetBlockID << " cargado en el slot " << freeSlot
            << " (" << tempBlock.recordCount << " registros).\n";
  return true;
}

bool RemoveBlockFromBuffer(int targetBlockID) {
  int idx = FindBlockInBuffer(targetBlockID);
  if (idx == -1) {
    std::cout << "El bloque " << targetBlockID << " no está en el buffer.\n";
    return false;
  }

  // Marcar el slot como vacío para liberar espacio lógico en el buffer
  size_t sIdx = static_cast<size_t>(idx);
  BUFFER_POOL[sIdx].blockID = -1;
  BUFFER_POOL[sIdx].recordCount = 0;
  BUFFER_POOL[sIdx].nextBlockID = -1;
  std::cout << "Bloque " << targetBlockID << " quitado del buffer (slot " << idx
            << " liberado).\n";
  return true;
}

void ShowLoadedBlockRecords(int targetBlockID) {
  int idx = FindBlockInBuffer(targetBlockID);
  if (idx == -1) {
    std::cout << "El bloque " << targetBlockID
              << " no está cargado en el buffer. Cárgalo primero.\n";
    return;
  }

  const DataBlock &blk = BUFFER_POOL[static_cast<size_t>(idx)];
  std::cout << "\n--- Registros del Bloque " << blk.blockID << " ("
            << blk.recordCount << " válidos) ---\n";
  // Solo iteramos hasta recordCount para no mostrar basura en bloques parciales
  for (int i = 0; i < blk.recordCount; i++) {
    size_t rIdx = static_cast<size_t>(i);
    std::cout << "[" << i << "] userID: " << blk.records[rIdx].userID
              << " | movieID: " << blk.records[rIdx].movieID
              << " | rating: " << std::fixed << std::setprecision(2)
              << blk.records[rIdx].rating << "\n";
  }
}
