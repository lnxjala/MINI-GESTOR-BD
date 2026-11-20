#pragma once
#include "RegistroRating.hpp"
#include <fstream>
#include <string>

class GestorDisco {
public:
    explicit GestorDisco(const std::string& rutaArchivo);
    ~GestorDisco();

    GestorDisco(const GestorDisco&) = delete;
    GestorDisco& operator=(const GestorDisco&) = delete;

    bool leerBloque(int idBloque, BloqueDatos& bloque);
    bool escribirBloque(const BloqueDatos& bloque);

    int numBloques() const { return totalBloques; }
    bool abierto() const { return archivo.is_open(); }

private:
    std::fstream archivo;
    std::string ruta;
    int totalBloques = 0;
};
