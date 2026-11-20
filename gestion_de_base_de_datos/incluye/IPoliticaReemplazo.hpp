#pragma once
#include "Marco.hpp"
#include <vector>

class IPoliticaReemplazo {
public:
    virtual ~IPoliticaReemplazo() = default;

    virtual int seleccionarVictima() = 0;

    virtual void alAcceder(int idMarco) = 0;
    virtual void alLiberar(int idMarco) = 0;
    virtual void alCargar(int idMarco) = 0;

    virtual const char* nombre() const = 0;
};
