#include "PoliticaReloj.hpp"

PoliticaReloj::PoliticaReloj(size_t numMarcos)
    : bitsReferencia(numMarcos, false), fijados(numMarcos, false) {}

int PoliticaReloj::seleccionarVictima() {
    const size_t n = bitsReferencia.size();
    if (n == 0) return -1;

    for (size_t pasos = 0; pasos < 2 * n; ++pasos) {
        if (!fijados[manecilla]) {
            if (bitsReferencia[manecilla]) {
                bitsReferencia[manecilla] = false;
            } else {
                int victima = static_cast<int>(manecilla);
                manecilla = (manecilla + 1) % n;
                return victima;
            }
        }
        manecilla = (manecilla + 1) % n;
    }
    return -1;
}

void PoliticaReloj::alAcceder(int idMarco) {
    bitsReferencia[idMarco] = true;
    fijados[idMarco] = true;
}

void PoliticaReloj::alLiberar(int idMarco) {
    fijados[idMarco] = false;
}

void PoliticaReloj::alCargar(int idMarco) {
    bitsReferencia[idMarco] = true;
    fijados[idMarco] = true;
}
