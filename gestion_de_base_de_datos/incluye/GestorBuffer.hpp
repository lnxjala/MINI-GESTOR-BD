#pragma once
#include "Marco.hpp"
#include "GestorDisco.hpp"
#include "IPoliticaReemplazo.hpp"
#include <vector>
#include <unordered_map>
#include <memory>

enum class TipoPolitica { LRU, MRU, Reloj };

class GestorBuffer {
public:
    GestorBuffer(GestorDisco& disco, size_t numMarcos, TipoPolitica tipo);
    ~GestorBuffer();

    GestorBuffer(const GestorBuffer&) = delete;
    GestorBuffer& operator=(const GestorBuffer&) = delete;

    Marco* fijarPagina(int idPagina);
    void liberarPagina(int idPagina, bool sucio);
    void volcarTodo();

    size_t aciertos() const { return hits; }
    size_t fallos()   const { return misses; }
    double tasaAciertos() const;
    const char* nombrePolitica() const { return politica->nombre(); }
    size_t numMarcos() const { return marcos.size(); }

    void imprimirEstado(const std::string& etiqueta = "") const;

private:
    GestorDisco& disco;
    std::vector<Marco> marcos;
    std::unordered_map<int, int> tablaPaginas;
    std::unique_ptr<IPoliticaReemplazo> politica;
    TipoPolitica tipoPolitica;
    size_t hits = 0;
    size_t misses = 0;
    size_t relojLogico = 0;

    int buscarMarcoVacio();
};
