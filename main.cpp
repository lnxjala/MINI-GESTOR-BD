#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Constante universal para el tamaño de página/bloque en disco (ej. 4096 o
// 1024)
constexpr size_t BLOCK_SIZE = 1024;

// Estructura ultracompacta para un registro de rating (12 bytes en total)
struct RatingRecord {
  int userID;   // 4 bytes
  int movieID;  // 4 bytes
  float rating; // 4 bytes
};

// Cálculo automático en tiempo de compilación según BLOCK_SIZE y RatingRecord
constexpr size_t HEADER_SIZE =
    sizeof(int) * 3; // blockID, recordCount, nextBlockID
constexpr size_t PAYLOAD_BYTES = BLOCK_SIZE - HEADER_SIZE;
constexpr size_t MAX_RECORDS_PER_BLOCK = PAYLOAD_BYTES / sizeof(RatingRecord);
constexpr size_t PADDING_BYTES =
    PAYLOAD_BYTES - (MAX_RECORDS_PER_BLOCK * sizeof(RatingRecord));

static_assert(
    MAX_RECORDS_PER_BLOCK > 0,
    "Error: BLOCK_SIZE es demasiado pequeño para almacenar registros.");

// Estructura del bloque físico de disco
struct DataBlock {
  int blockID;     // Identificador único del bloque (-1 si el slot está libre)
  int recordCount; // Cuántos registros válidos contiene este bloque
  int nextBlockID; // Puntero lógico al siguiente bloque (-1 si es el último)

  RatingRecord records[MAX_RECORDS_PER_BLOCK];
  char padding[PADDING_BYTES > 0 ? PADDING_BYTES : 1];
};

static_assert(sizeof(DataBlock) == BLOCK_SIZE,
              "Error: DataBlock no mide exactamente BLOCK_SIZE bytes.");

bool writeBlock(std::ofstream &outFile, const DataBlock &block) {
  std::streamoff offset =
      static_cast<std::streamoff>(block.blockID) * BLOCK_SIZE;

  outFile.seekp(offset, std::ios::beg);
  if (!outFile.good()) {
    std::cerr << "ERROR: Falló seekp al escribir el bloque " << block.blockID
              << "\n";
    return false;
  }

  outFile.write(reinterpret_cast<const char *>(&block), BLOCK_SIZE);
  if (!outFile.good()) {
    std::cerr << "ERROR: Falló la escritura en disco del bloque "
              << block.blockID << "\n";
    return false;
  }

  return true;
}

bool readBlock(std::ifstream &inFile, DataBlock &outBlock, int blockID) {
  if (blockID < 0) {
    return false;
  }

  std::streamoff offset = static_cast<std::streamoff>(blockID) * BLOCK_SIZE;

  inFile.seekg(offset, std::ios::beg);
  if (!inFile.good()) {
    return false;
  }

  inFile.read(reinterpret_cast<char *>(&outBlock), BLOCK_SIZE);
  if (inFile.gcount() != static_cast<std::streamsize>(BLOCK_SIZE)) {
    return false;
  }

  return true;
}

const int BUFFER_CAPACITY = 6;
DataBlock BUFFER_POOL[BUFFER_CAPACITY];

void InitBuffer() {
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    BUFFER_POOL[i].blockID = -1; // -1 indica slot vacío
    BUFFER_POOL[i].recordCount = 0;
    BUFFER_POOL[i].nextBlockID = -1;
  }
}

int CountLoadedBlocks() {
  int count = 0;
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (BUFFER_POOL[i].blockID != -1) {
      count++;
    }
  }
  return count;
}

int FindBlockInBuffer(int targetBlockID) {
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (BUFFER_POOL[i].blockID == targetBlockID) {
      return i;
    }
  }
  return -1;
}

int FindFreeSlot() {
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (BUFFER_POOL[i].blockID == -1) {
      return i;
    }
  }
  return -1;
}

void ShowBufferStatus(int totalDiskBlocks) {
  int loaded = CountLoadedBlocks();
  int available = BUFFER_CAPACITY - loaded;

  std::cout << "\n--- ESTADO DEL BUFFER POOL ---\n";
  std::cout << "Bloques creados en disco : " << totalDiskBlocks << " ("
            << BLOCK_SIZE << " bytes c/u)\n";
  std::cout << "Capacidad del buffer     : " << BUFFER_CAPACITY << " bloques\n";
  std::cout << "Bloques cargados         : " << loaded << "\n";
  std::cout << "IDs cargados             : ";
  bool any = false;
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (BUFFER_POOL[i].blockID != -1) {
      std::cout << BUFFER_POOL[i].blockID << " ";
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
                       int totalDiskBlocks) {
  if (targetBlockID < 0 || targetBlockID >= totalDiskBlocks) {
    std::cout << "Ese blockID no existe. Rango válido: 0 a "
              << (totalDiskBlocks > 0 ? totalDiskBlocks - 1 : 0) << ".\n";
    return false;
  }

  int existingIdx = FindBlockInBuffer(targetBlockID);
  if (existingIdx != -1) {
    std::cout << "El bloque " << targetBlockID << " ya está cargado en el slot "
              << existingIdx << ".\n";
    return true;
  }

  int freeSlot = FindFreeSlot();
  if (freeSlot == -1) {
    std::cout << "Buffer lleno: no hay espacio disponible (" << BUFFER_CAPACITY
              << "/" << BUFFER_CAPACITY << "). Quita un bloque primero.\n";
    return false;
  }

  DataBlock tempBlock;
  if (!readBlock(binFile, tempBlock, targetBlockID)) {
    std::cout << "No se pudo leer el bloque " << targetBlockID
              << " desde el archivo binario.\n";
    return false;
  }

  BUFFER_POOL[freeSlot] = tempBlock;
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

  BUFFER_POOL[idx].blockID = -1;
  BUFFER_POOL[idx].recordCount = 0;
  BUFFER_POOL[idx].nextBlockID = -1;
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

  const DataBlock &blk = BUFFER_POOL[idx];
  std::cout << "\n--- Registros del Bloque " << blk.blockID << " ("
            << blk.recordCount << " válidos) ---\n";
  for (int i = 0; i < blk.recordCount; i++) {
    std::cout << "[" << i << "] userID: " << blk.records[i].userID
              << " | movieID: " << blk.records[i].movieID
              << " | rating: " << std::fixed << std::setprecision(2)
              << blk.records[i].rating << "\n";
  }
}

int main() {
  InitBuffer();

  std::ifstream tsvFile("ratings.tsv");
  if (!tsvFile.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo abrir 'ratings.tsv'.\n";
    return 1;
  }

  std::ofstream binFile("ratings.bin", std::ios::binary);
  if (!binFile.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo crear el archivo binario "
                 "'ratings.bin'.\n";
    return 1;
  }

  std::string line;
  if (!std::getline(tsvFile, line)) {
    std::cerr << "El archivo TSV está vacío.\n";
    return 1;
  }

  DataBlock currentBlock;
  int currentBlockID = 0;
  currentBlock.blockID = currentBlockID;
  currentBlock.recordCount = 0;
  currentBlock.nextBlockID = -1;

  size_t totalRecordsProcessed = 0;

  while (std::getline(tsvFile, line)) {
    if (line.empty())
      continue;

    std::stringstream ss(line);
    std::string userStr, movieStr, ratingStr, timestampStr;

    if (!std::getline(ss, userStr, '\t') || !std::getline(ss, movieStr, '\t') ||
        !std::getline(ss, ratingStr, '\t')) {
      continue;
    }

    RatingRecord record;
    record.userID = static_cast<int>(std::stoul(userStr));
    record.movieID = static_cast<int>(std::stoul(movieStr));
    record.rating = std::stof(ratingStr);

    currentBlock.records[currentBlock.recordCount] = record;
    currentBlock.recordCount++;
    totalRecordsProcessed++;

    if (currentBlock.recordCount == static_cast<int>(MAX_RECORDS_PER_BLOCK)) {
      currentBlock.nextBlockID = currentBlockID + 1;
      if (!writeBlock(binFile, currentBlock)) {
        return 1;
      }
      currentBlockID++;
      currentBlock.blockID = currentBlockID;
      currentBlock.recordCount = 0;
      currentBlock.nextBlockID = -1;
    }
  }

  if (currentBlock.recordCount > 0) {
    currentBlock.nextBlockID = -1;
    if (!writeBlock(binFile, currentBlock)) {
      return 1;
    }
    currentBlockID++;
  }

  int totalDiskBlocks = currentBlockID;

  tsvFile.close();
  binFile.close();

  std::cout << "¡Conversión completada con éxito!\n";
  std::cout << "Total de registros procesados: " << totalRecordsProcessed
            << "\n";
  std::cout << "Total de bloques creados (" << BLOCK_SIZE
            << " bytes): " << totalDiskBlocks << "\n";
  std::cout << "Capacidad máxima por bloque: " << MAX_RECORDS_PER_BLOCK
            << " registros\n";

  std::ifstream binFileCreated("ratings.bin", std::ios::binary);
  if (!binFileCreated.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo reabrir 'ratings.bin'.\n";
    return 1;
  }

  int option = -1;
  while (true) {
    ShowBufferStatus(totalDiskBlocks);
    std::cout << "1. Cargar bloque al buffer\n";
    std::cout << "2. Quitar bloque del buffer\n";
    std::cout << "3. Ver registros de un bloque cargado\n";
    std::cout << "0. Salir\n";
    std::cout << "Elige una opción: ";

    if (!(std::cin >> option)) {
      std::cin.clear();
      std::cin.ignore(10000, '\n');
      std::cout << "Entrada inválida.\n";
      continue;
    }

    if (option == 0) {
      break;
    } else if (option == 1) {
      int id;
      std::cout << "Ingresa el blockID a cargar: ";
      if (std::cin >> id) {
        LoadBlockToBuffer(binFileCreated, id, totalDiskBlocks);
      }
    } else if (option == 2) {
      int id;
      std::cout << "Ingresa el blockID a quitar: ";
      if (std::cin >> id) {
        RemoveBlockFromBuffer(id);
      }
    } else if (option == 3) {
      int id;
      std::cout << "Ingresa el blockID cargado para ver su información: ";
      if (std::cin >> id) {
        ShowLoadedBlockRecords(id);
      }
    } else {
      std::cout << "Opción no válida.\n";
    }
  }

  return 0;
}
