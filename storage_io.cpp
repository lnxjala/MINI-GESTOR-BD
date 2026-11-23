#include "storage_io.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

// Función robusta para escribir un bloque en el archivo binario en su offset
// exacto
bool writeBlock(std::ofstream &outFile, const DataBlock &block,
                size_t blockSize) {
  // Calculamos la posición exacta en bytes: (ID del Bloque) * (Tamaño del
  // Bloque)
  std::streamoff offset = static_cast<std::streamoff>(block.blockID) *
                          static_cast<std::streamoff>(blockSize);

  outFile.seekp(offset, std::ios::beg);
  if (!outFile.good()) {
    std::cerr << "ERROR: Falló seekp al escribir el bloque " << block.blockID
              << "\n";
    return false;
  }

  // Armamos un buffer exacto de 'blockSize' bytes para garantizar que cada
  // bloque en disco ocupe siempre exactamente 'blockSize' bytes (cabecera +
  // registros + padding en cero)
  std::vector<char> rawBlock(blockSize, 0);
  size_t pos = 0;

  // 1. Copiar cabecera (blockID, recordCount, nextBlockID)
  std::memcpy(rawBlock.data() + pos, &block.blockID, sizeof(int));
  pos += sizeof(int);
  std::memcpy(rawBlock.data() + pos, &block.recordCount, sizeof(int));
  pos += sizeof(int);
  std::memcpy(rawBlock.data() + pos, &block.nextBlockID, sizeof(int));
  pos += sizeof(int);

  // 2. Copiar los registros válidos sin exceder el espacio útil del bloque
  size_t maxBytesForRecords =
      (blockSize > HEADER_SIZE) ? (blockSize - HEADER_SIZE) : 0;
  size_t bytesToCopy = block.records.size() * sizeof(RatingRecord);
  if (bytesToCopy > maxBytesForRecords) {
    bytesToCopy = maxBytesForRecords;
  }

  if (bytesToCopy > 0 && !block.records.empty()) {
    std::memcpy(rawBlock.data() + pos, block.records.data(), bytesToCopy);
  }

  // 3. Escribir los 'blockSize' bytes de golpe en disco
  outFile.write(rawBlock.data(), static_cast<std::streamsize>(blockSize));
  if (!outFile.good()) {
    std::cerr << "ERROR: Falló la escritura en disco del bloque "
              << block.blockID << "\n";
    return false;
  }

  return true;
}

bool readBlock(std::ifstream &inFile, DataBlock &outBlock, int blockID,
               size_t blockSize, size_t maxRecordsPerBlock) {
  if (blockID < 0) {
    return false;
  }

  // 1. Calcular el desplazamiento (offset) exacto en bytes
  std::streamoff offset = static_cast<std::streamoff>(blockID) *
                          static_cast<std::streamoff>(blockSize);

  // 2. Mover el puntero de lectura (*get pointer*) a esa posición exacta
  inFile.seekg(offset, std::ios::beg);
  if (!inFile.good()) {
    return false;
  }

  // 3. Leer únicamente los 'blockSize' bytes correspondientes a este bloque
  std::vector<char> rawBlock(blockSize, 0);
  inFile.read(rawBlock.data(), static_cast<std::streamsize>(blockSize));

  // 4. Verificar si se leyeron exactamente los 'blockSize' bytes esperados
  if (inFile.gcount() != static_cast<std::streamsize>(blockSize)) {
    // Si gcount() es menor, significa que el bloque no existe
    // o intentaste leer más allá del final del archivo.
    return false;
  }

  // Reconstruir cabecera y registros en la estructura DataBlock
  size_t pos = 0;
  std::memcpy(&outBlock.blockID, rawBlock.data() + pos, sizeof(int));
  pos += sizeof(int);
  std::memcpy(&outBlock.recordCount, rawBlock.data() + pos, sizeof(int));
  pos += sizeof(int);
  std::memcpy(&outBlock.nextBlockID, rawBlock.data() + pos, sizeof(int));
  pos += sizeof(int);

  outBlock.records.assign(maxRecordsPerBlock, RatingRecord{0, 0, 0.0f});
  size_t validCount = (outBlock.recordCount > 0)
                          ? static_cast<size_t>(outBlock.recordCount)
                          : 0;
  if (validCount > maxRecordsPerBlock) {
    validCount = maxRecordsPerBlock;
  }

  if (validCount > 0) {
    std::memcpy(outBlock.records.data(), rawBlock.data() + pos,
                validCount * sizeof(RatingRecord));
  }

  return true;
}

bool ConvertTsvToBin(const std::string &tsvPath, const std::string &binPath,
                     size_t blockSize, size_t maxRecordsPerBlock,
                     int &outTotalBlocks, size_t &outTotalRecords) {
  // 1. Abrir archivo TSV de origen en modo texto
  std::ifstream tsvFile(tsvPath);
  if (!tsvFile.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo abrir '" << tsvPath << "'.\n";
    return false;
  }

  // 2. Abrir archivo binario de destino para escritura (crea o sobrescribe)
  std::ofstream binFile(binPath, std::ios::binary);
  if (!binFile.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo crear '" << binPath << "'.\n";
    return false;
  }

  std::string line;
  // Descartar la cabecera del TSV (userId \t movieId \t rating \t timestamp)
  if (!std::getline(tsvFile, line)) {
    std::cerr << "El archivo TSV está vacío.\n";
    return false;
  }

  DataBlock currentBlock;
  int currentBlockID = 0;

  // Inicializamos el primer bloque con la capacidad calculada según 'blockSize'
  currentBlock.blockID = currentBlockID;
  currentBlock.recordCount = 0;
  currentBlock.nextBlockID = -1;
  currentBlock.records.assign(maxRecordsPerBlock, RatingRecord{0, 0, 0.0f});

  outTotalRecords = 0;

  // 3. Procesamiento línea por línea de forma secuencial
  while (std::getline(tsvFile, line)) {
    if (line.empty())
      continue;

    std::stringstream ss(line);
    std::string userStr, movieStr, ratingStr;

    // Extraemos las columnas del TSV (user, movie, rating)
    if (!std::getline(ss, userStr, '\t') || !std::getline(ss, movieStr, '\t') ||
        !std::getline(ss, ratingStr, '\t')) {
      continue; // Saltar líneas mal formateadas
    }

    // Parseo a los tipos de datos de la estructura
    RatingRecord record;
    record.userID = static_cast<int>(std::stoul(userStr));
    record.movieID = static_cast<int>(std::stoul(movieStr));
    record.rating = std::stof(ratingStr);

    // Insertar el registro en el bloque actual en memoria
    currentBlock.records[static_cast<size_t>(currentBlock.recordCount)] =
        record;
    currentBlock.recordCount++;
    outTotalRecords++;

    // 4. ¿Se llenó el bloque según el tamaño elegido? Es hora de volcarlo a
    // disco
    if (static_cast<size_t>(currentBlock.recordCount) == maxRecordsPerBlock) {
      // Enlazamos el puntero lógico al siguiente bloque
      currentBlock.nextBlockID = currentBlockID + 1;
      if (!writeBlock(binFile, currentBlock, blockSize)) {
        return false;
      }

      // Preparamos el siguiente bloque en memoria
      currentBlockID++;
      currentBlock.blockID = currentBlockID;
      currentBlock.recordCount = 0;
      currentBlock.nextBlockID = -1;
      currentBlock.records.assign(maxRecordsPerBlock, RatingRecord{0, 0, 0.0f});
    }
  }

  // 5. Guardar el último bloque incompleto (si quedaron registros remanentes)
  if (currentBlock.recordCount > 0) {
    currentBlock.nextBlockID = -1; // Es el último bloque del archivo
    if (!writeBlock(binFile, currentBlock, blockSize)) {
      return false;
    }
    currentBlockID++;
  }

  outTotalBlocks = currentBlockID;
  return true;
}
