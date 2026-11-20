#include "GestorDisco.hpp"
#include <iostream>

GestorDisco::GestorDisco(const std::string& rutaArchivo) : ruta(rutaArchivo) {
    archivo.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: No se pudo abrir '" << ruta << "'. "
                  << "Debe existir; usa primero la herramienta tsv_a_bin.\n";
        return;
    }
    archivo.seekg(0, std::ios::end);
    std::streamoff tam = archivo.tellg();
    totalBloques = static_cast<int>(tam / TAMANIO_BLOQUE);
}

GestorDisco::~GestorDisco() {
    if (archivo.is_open()) archivo.close();
}

bool GestorDisco::leerBloque(int idBloque, BloqueDatos& bloque) {
    if (!archivo.is_open()) return false;
    std::streamoff offset = static_cast<std::streamoff>(idBloque) * TAMANIO_BLOQUE;
    archivo.clear();
    archivo.seekg(offset, std::ios::beg);
    if (!archivo.good()) return false;
    archivo.read(reinterpret_cast<char*>(&bloque), TAMANIO_BLOQUE);
    return archivo.gcount() == static_cast<std::streamsize>(TAMANIO_BLOQUE);
}

bool GestorDisco::escribirBloque(const BloqueDatos& bloque) {
    if (!archivo.is_open()) return false;
    std::streamoff offset = static_cast<std::streamoff>(bloque.idBloque) * TAMANIO_BLOQUE;
    archivo.clear();
    archivo.seekp(offset, std::ios::beg);
    if (!archivo.good()) return false;
    archivo.write(reinterpret_cast<const char*>(&bloque), TAMANIO_BLOQUE);
    archivo.flush();
    if (bloque.idBloque + 1 > totalBloques) totalBloques = bloque.idBloque + 1;
    return archivo.good();
}
