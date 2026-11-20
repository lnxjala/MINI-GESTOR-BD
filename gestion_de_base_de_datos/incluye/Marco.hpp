#pragma once
#include "RegistroRating.hpp"

struct Marco {
    BloqueDatos bloque{};
    int idMarco = -1;
    int idPagina = -1;
    bool bitSucio = false;
    int contadorFijacion = 0;
    size_t ultimoUso = 0;
    bool bitReferencia = false;

    bool estaVacio() const { return idPagina == -1; }
    bool estaFijada() const { return contadorFijacion > 0; }
};
