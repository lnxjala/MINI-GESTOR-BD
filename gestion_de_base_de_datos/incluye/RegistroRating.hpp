#pragma once
#include <cstdint>
#include <cstddef>

constexpr size_t TAMANIO_BLOQUE = 4096;

struct RegistroRating {
    uint32_t idUsuario;
    uint32_t idPelicula;
    float rating;
};

struct BloqueDatos {
    int32_t idBloque;
    int32_t numRegistros;
    int32_t idSiguienteBloque;

    static constexpr size_t TAMANIO_CABECERA = 3 * sizeof(int32_t);
    static constexpr size_t MAX_REGISTROS =
        (TAMANIO_BLOQUE - TAMANIO_CABECERA) / sizeof(RegistroRating);
    static constexpr size_t TAMANIO_RELLENO =
        TAMANIO_BLOQUE - TAMANIO_CABECERA - MAX_REGISTROS * sizeof(RegistroRating);

    RegistroRating registros[MAX_REGISTROS];
    char relleno[TAMANIO_RELLENO];
};

static_assert(sizeof(BloqueDatos) == TAMANIO_BLOQUE,
              "BloqueDatos debe medir exactamente 4096 bytes.");
