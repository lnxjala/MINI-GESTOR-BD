#include "PoliticaMRU.hpp"

void PoliticaMRU::quitar(int idMarco) {
    auto it = indice.find(idMarco);
    if (it == indice.end()) return;
    pila.erase(it->second);
    indice.erase(it);
}

void PoliticaMRU::agregarAlTope(int idMarco) {
    if (indice.find(idMarco) != indice.end()) return;
    pila.push_back(idMarco);
    indice[idMarco] = std::prev(pila.end());
}

int PoliticaMRU::seleccionarVictima() {
    if (pila.empty()) return -1;
    int idMarco = pila.back();
    pila.pop_back();
    indice.erase(idMarco);
    return idMarco;
}

void PoliticaMRU::alAcceder(int idMarco) { quitar(idMarco); }
void PoliticaMRU::alLiberar(int idMarco) { agregarAlTope(idMarco); }
void PoliticaMRU::alCargar(int /*idMarco*/) {}
