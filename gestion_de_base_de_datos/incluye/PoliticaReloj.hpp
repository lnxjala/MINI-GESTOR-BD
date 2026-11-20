#pragma once
#include "IPoliticaReemplazo.hpp"
#include <vector>

class PoliticaReloj : public IPoliticaReemplazo {
public:
    explicit PoliticaReloj(size_t numMarcos);

    int seleccionarVictima() override;
    void alAcceder(int idMarco) override;
    void alLiberar(int idMarco) override;
    void alCargar(int idMarco) override;

    const char* nombre() const override { return "Reloj"; }

    bool bitReferencia(int idMarco) const { return bitsReferencia[idMarco]; }
    size_t posicionManecilla() const { return manecilla; }

private:
    std::vector<bool> bitsReferencia;
    std::vector<bool> fijados;
    size_t manecilla = 0;
};
