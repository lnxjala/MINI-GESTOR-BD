#include "blockTypes.h"
#include "bufferPool.h"
#include "storage_io.h"
#include <cstddef>
#include <fstream>
#include <iostream>

int main() {
  size_t blockSize = 0;

  // Pedimos primero el tamaño de bloque que indique la profesora (ej. 1024 o
  // 4096)
  while (true) {
    std::cout << "Ingresa el tamaño de bloque en bytes (ej. 1024, 4096): ";
    if (std::cin >> blockSize && blockSize > HEADER_SIZE) {
      size_t maxRec = CalcMaxRecordsPerBlock(blockSize);
      if (maxRec > 0) {
        break;
      }
    }
    std::cin.clear();
    std::cin.ignore(10000, '\n');
    std::cout << "Tamaño inválido. Debe ser mayor que " << HEADER_SIZE
              << " bytes y permitir al menos 1 registro ("
              << sizeof(RatingRecord) << " bytes).\n";
  }

  size_t maxRecordsPerBlock = CalcMaxRecordsPerBlock(blockSize);

  // Dejamos el buffer vacío al inicio con la capacidad de registros calculada
  InitBuffer(maxRecordsPerBlock);

  int totalDiskBlocks = 0;
  size_t totalRecordsProcessed = 0;

  // Convertimos ratings.tsv a ratings.bin en bloques del tamaño ingresado
  if (!ConvertTsvToBin("ratings.tsv", "ratings.bin", blockSize,
                       maxRecordsPerBlock, totalDiskBlocks,
                       totalRecordsProcessed)) {
    return 1;
  }

  std::cout << "\n¡Conversión completada con éxito!\n";
  std::cout << "Tamaño de bloque configurado : " << blockSize << " bytes\n";
  std::cout << "Registros máximos por bloque : " << maxRecordsPerBlock << "\n";
  std::cout << "Total de registros procesados: " << totalRecordsProcessed
            << "\n";
  std::cout << "Total de bloques creados     : " << totalDiskBlocks << "\n";

  // Reabrimos el archivo binario en modo lectura para consultar bloques bajo
  // demanda
  std::ifstream binFileCreated("ratings.bin", std::ios::binary);
  if (!binFileCreated.is_open()) {
    std::cerr << "ERROR CRÍTICO: No se pudo reabrir 'ratings.bin'.\n";
    return 1;
  }

  // Menú interactivo para probar carga, descarga y lectura de bloques
  int option = -1;
  while (true) {
    ShowBufferStatus(totalDiskBlocks, blockSize, maxRecordsPerBlock);
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
        LoadBlockToBuffer(binFileCreated, id, totalDiskBlocks, blockSize,
                          maxRecordsPerBlock);
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
