#pragma once
#include "IPoliticaReemplazo.hpp"
#include <list>
#include <unordered_map>

class PoliticaLRU : public IPoliticaReemplazo {
public:
    PoliticaLRU() = default;

    int seleccionarVictima() override;
    void alAcceder(int idMarco) override;
    void alLiberar(int idMarco) override;
    void alCargar(int idMarco) override;

    const char* nombre() const override { return "LRU"; }

private:
    std::list<int> cola;
    std::unordered_map<int, std::list<int>::iterator> indice;

    void quitar(int idMarco);
    void agregarAlFinal(int idMarco);
};
