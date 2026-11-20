#include "PoliticaLRU.hpp"

void PoliticaLRU::quitar(int idMarco) {
    auto it = indice.find(idMarco);
    if (it == indice.end()) return;
    cola.erase(it->second);
    indice.erase(it);
}

void PoliticaLRU::agregarAlFinal(int idMarco) {
    if (indice.find(idMarco) != indice.end()) return;
    cola.push_back(idMarco);
    indice[idMarco] = std::prev(cola.end());
}

int PoliticaLRU::seleccionarVictima() {
    if (cola.empty()) return -1;
    int idMarco = cola.front();
    cola.pop_front();
    indice.erase(idMarco);
    return idMarco;
}

void PoliticaLRU::alAcceder(int idMarco) { quitar(idMarco); }
void PoliticaLRU::alLiberar(int idMarco) { agregarAlFinal(idMarco); }
void PoliticaLRU::alCargar(int /*idMarco*/) {}
