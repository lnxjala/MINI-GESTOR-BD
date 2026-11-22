#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cassert>

// Constante universal para el tamaño de página/bloque en disco (4 KB)
constexpr size_t BLOCK_SIZE = 4096;

// Estructura ultracompacta para un registro de rating (12 bytes en total)
struct RatingRecord {
    uint32_t userID;  // 4 bytes
    uint32_t movieID; // 4 bytes
    float rating;     // 4 bytes
};

// Estructura del bloque físico de disco
struct DataBlock {
    int32_t blockID;     // 4 bytes: Identificador único del bloque
    int32_t recordCount; // 4 bytes: Cuántos registros válidos contiene este bloque
    int32_t nextBlockID; // 4 bytes: Puntero lógico al siguiente bloque (-1 si es el último)
                         // --- Total Cabecera: 12 bytes ---

    // Espacio útil para registros:
    // Quedan 4096 - 12 = 4084 bytes disponibles.
    // 4084 / 12 (tamaño de RatingRecord) = 340.33 -> 340 registros exactos.
    // 340 * 12 = 4080 bytes utilizados por los registros.
    RatingRecord records[340];

    // Relleno (Padding) para forzar que el struct mida exactamente 4096 bytes:
    // 4084 - 4080 = 4 bytes de padding.
    char padding[4];
};

// Verificación estricta en tiempo de compilación. 
// Si por alineación de memoria del compilador esto falla, el programa no compila.
static_assert(sizeof(DataBlock) == BLOCK_SIZE, "Error: DataBlock no mide exactamente 4096 bytes.");

// Función robusta para escribir un bloque en el archivo binario en su offset exacto
bool writeBlock(std::ofstream& outFile, const DataBlock& block) {
    // Calculamos la posición exacta en bytes: (ID del Bloque) * (Tamaño del Bloque)
    std::streamoff offset = static_cast<std::streamoff>(block.blockID) * BLOCK_SIZE;
    
    outFile.seekp(offset, std::ios::beg);
    if (!outFile.good()) {
        std::cerr << "ERROR: Falló seekp al escribir el bloque " << block.blockID << "\n";
        return false;
    }

    // Escribimos los 4096 bytes de golpe casteando la estructura a un puntero de caracteres
    outFile.write(reinterpret_cast<const char*>(&block), BLOCK_SIZE);
    
    if (!outFile.good()) {
        std::cerr << "ERROR: Falló la escritura en disco del bloque " << block.blockID << "\n";
        return false;
    }
    
    return true;
}

bool readBlock(std::ifstream& inFile, DataBlock &outBlock, int blockID) {

    // 1. Calcular el desplazamiento (offset) exacto en bytes
    std::streamoff offset = static_cast<std::streamoff>(blockID) * BLOCK_SIZE;

    // 2. Mover el puntero de lectura (*get pointer*) a esa posición exacta
    inFile.seekg(offset, std::ios::beg);
    if (!inFile.good()) {
        std::cerr << "ERROR: Falló seekg al intentar leer el bloque " << blockID << "\n";
        return false;
    }

    // 3. Leer los 4096 bytes de golpe directamente hacia nuestra estructura en RAM
    inFile.read(reinterpret_cast<char*>(&outBlock), BLOCK_SIZE);

    // 4. Verificar si se leyeron exactamente los 4096 bytes esperados
    if (inFile.gcount() != BLOCK_SIZE) {
        // Si gcount() es menor, significa que el bloque no existe 
        // o intentaste leer más allá del final del archivo.
        return false;
    }

    return true;
}

const int BUFFER_CAPACITY = 6;
DataBlock BUFFER_POOL[BUFFER_CAPACITY];

void ShowBufferIndices() {
    for (auto block : BUFFER_POOL) {
        std::cout << "BlockID: " << block.blockID << "\n";
    }
}

void ReadFromBufferID(int id) {
    for (auto record : BUFFER_POOL[id].records) {
        std::cout << "Rating: " << std::fixed << std::setprecision(2) << record.rating << "\n";
    }
}


int main() {
    // 1. Abrir archivo TSV de origen en modo texto
    std::ifstream tsvFile("ratings.tsv");
    if (!tsvFile.is_open()) {
        std::cerr << "ERROR CRÍTICO: No se pudo abrir 'ratings.tsv'.\n";
        return 1;
    }

    // 2. Abrir archivo binario de destino para escritura (crea o sobrescribe)
    std::ofstream binFile("ratings.bin", std::ios::binary);
    if (!binFile.is_open()) {
        std::cerr << "ERROR CRÍTICO: No se pudo crear el archivo binario 'ratings.bin'.\n";
        return 1;
    }

    std::string line;
    // Opcional: Descartar la cabecera del TSV (userId \t movieId \t rating \t timestamp)
    if (!std::getline(tsvFile, line)) {
        std::cerr << "El archivo TSV está vacío.\n";
        return 1;
    }

    DataBlock currentBlock;
    int32_t currentBlockID = 0;

    // Inicializamos el primer bloque
    currentBlock.blockID = currentBlockID;
    currentBlock.recordCount = 0;
    currentBlock.nextBlockID = -1;

    size_t totalRecordsProcessed = 0;

    // 3. Procesamiento línea por línea de forma secuencial
    while (std::getline(tsvFile, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string userStr, movieStr, ratingStr, timestampStr;

        // Extraemos las columnas del TSV (asumiendo 3 o 4 columnas: user, movie, rating, [timestamp])
        if (!std::getline(ss, userStr, '\t') ||
            !std::getline(ss, movieStr, '\t') ||
            !std::getline(ss, ratingStr, '\t')) {
            continue; // Saltar líneas mal formateadas
        }

        // Parseo seguro a los tipos de datos nativos
        RatingRecord record;
        record.userID = static_cast<uint32_t>(std::stoul(userStr));
        record.movieID = static_cast<uint32_t>(std::stoul(movieStr));
        record.rating = std::stof(ratingStr);

        // Insertar el registro en el bloque actual en memoria
        currentBlock.records[currentBlock.recordCount] = record;
        currentBlock.recordCount++;
        totalRecordsProcessed++;

        // 4. ¿Se llenó el bloque? Es hora de volcarlo a disco
        if (currentBlock.recordCount == 340) {
            // Si habrá un siguiente bloque, enlazamos el puntero lógico
            currentBlock.nextBlockID = currentBlockID + 1;

            if (!writeBlock(binFile, currentBlock)) {
                return 1;
            }

            // Preparamos el siguiente bloque en memoria
            currentBlockID++;
            currentBlock.blockID = currentBlockID;
            currentBlock.recordCount = 0;
            currentBlock.nextBlockID = -1;
        }
    }

    // 5. Guardar el último bloque incompleto (si quedaron registros remanentes)
    if (currentBlock.recordCount > 0) {
        currentBlock.nextBlockID = -1; // Es el último bloque del archivo
        if (!writeBlock(binFile, currentBlock)) {
            return 1;
        }
        currentBlockID++;
    }

    std::cout << "Total Blocks: " << currentBlock.blockID << "\n";

    // Cierre limpio de archivos
    tsvFile.close();
    binFile.close();

    std::cout << "¡Conversión completada con éxito!\n";
    std::cout << "Total de registros procesados: " << totalRecordsProcessed << "\n";
    std::cout << "Total de bloques de 4KB escritos: " << currentBlockID << "\n";

    std::ifstream binFileCreated("ratings.bin", std::ios::binary);

    for (int i = 0; i < 6; i++) {
        DataBlock dataBlock;
        readBlock(binFileCreated, dataBlock, i);        
        BUFFER_POOL[i] = dataBlock; 
    }

    ShowBufferIndices();
    ReadFromBufferID(2);

    return 0;
}