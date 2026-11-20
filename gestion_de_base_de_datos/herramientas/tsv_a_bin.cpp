#include "../incluye/RegistroRating.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>

static bool escribirBloque(std::fstream& archivo, const BloqueDatos& bloque) {
    std::streamoff offset = static_cast<std::streamoff>(bloque.idBloque) * TAMANIO_BLOQUE;
    archivo.seekp(offset, std::ios::beg);
    if (!archivo.good()) return false;
    archivo.write(reinterpret_cast<const char*>(&bloque), TAMANIO_BLOQUE);
    return archivo.good();
}

int main(int argc, char** argv) {
    const std::string entrada = (argc > 1) ? argv[1] : "datos/ratings.tsv";
    const std::string salida  = (argc > 2) ? argv[2] : "datos/ratings.bin";

    std::ifstream tsv(entrada);
    if (!tsv.is_open()) {
        std::cerr << "ERROR: No se pudo abrir '" << entrada << "'.\n";
        return 1;
    }

    std::fstream bin(salida, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!bin.is_open()) {
        std::cerr << "ERROR: No se pudo crear '" << salida << "'.\n";
        return 1;
    }
    bin.close();
    bin.open(salida, std::ios::in | std::ios::out | std::ios::binary);
    if (!bin.is_open()) {
        std::cerr << "ERROR: No se pudo reabrir '" << salida << "'.\n";
        return 1;
    }

    std::string linea;
    if (!std::getline(tsv, linea)) {
        std::cerr << "El TSV esta vacio.\n";
        return 1;
    }

    BloqueDatos bloqueActual{};
    int32_t idActual = 0;
    bloqueActual.idBloque = idActual;
    bloqueActual.numRegistros = 0;
    bloqueActual.idSiguienteBloque = -1;

    size_t totalRegistros = 0;

    while (std::getline(tsv, linea)) {
        if (linea.empty()) continue;

        std::stringstream ss(linea);
        std::string u, p, r;
        if (!std::getline(ss, u, '\t') ||
            !std::getline(ss, p, '\t') ||
            !std::getline(ss, r, '\t')) continue;

        RegistroRating reg{};
        try {
            reg.idUsuario  = static_cast<uint32_t>(std::stoul(u));
            reg.idPelicula = static_cast<uint32_t>(std::stoul(p));
            reg.rating     = std::stof(r);
        } catch (...) { continue; }

        bloqueActual.registros[bloqueActual.numRegistros++] = reg;
        totalRegistros++;

        if (bloqueActual.numRegistros == static_cast<int32_t>(BloqueDatos::MAX_REGISTROS)) {
            bloqueActual.idSiguienteBloque = -1;
            if (!escribirBloque(bin, bloqueActual)) {
                std::cerr << "ERROR escribiendo bloque " << idActual << ".\n";
                return 1;
            }
            ++idActual;
            bloqueActual = BloqueDatos{};
            bloqueActual.idBloque = idActual;
            bloqueActual.numRegistros = 0;
            bloqueActual.idSiguienteBloque = -1;
        }
    }

    if (bloqueActual.numRegistros > 0) {
        bloqueActual.idSiguienteBloque = -1;
        if (!escribirBloque(bin, bloqueActual)) return 1;
        ++idActual;
    }

    for (int32_t i = 0; i + 1 < idActual; ++i) {
        std::streamoff off = static_cast<std::streamoff>(i) * TAMANIO_BLOQUE
                           + offsetof(BloqueDatos, idSiguienteBloque);
        int32_t siguiente = i + 1;
        bin.seekp(off, std::ios::beg);
        bin.write(reinterpret_cast<const char*>(&siguiente), sizeof(int32_t));
    }

    bin.close();
    tsv.close();

    std::cout << "OK: " << totalRegistros << " registros en "
              << idActual << " bloques de " << TAMANIO_BLOQUE << " bytes.\n";
    return 0;
}
