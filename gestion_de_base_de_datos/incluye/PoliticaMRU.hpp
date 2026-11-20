#pragma once
#include "IPoliticaReemplazo.hpp"
#include <list>
#include <unordered_map>

class PoliticaMRU : public IPoliticaReemplazo {
public:
    PoliticaMRU() = default;

    int seleccionarVictima() override;
    void alAcceder(int idMarco) override;
    void alLiberar(int idMarco) override;
    void alCargar(int idMarco) override;

    const char* nombre() const override { return "MRU"; }

private:
    std::list<int> pila;
    std::unordered_map<int, std::list<int>::iterator> indice;

    void quitar(int idMarco);
    void agregarAlTope(int idMarco);
};
