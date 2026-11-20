#include "GestorBuffer.hpp"
#include "PoliticaLRU.hpp"
#include "PoliticaMRU.hpp"
#include "PoliticaReloj.hpp"
#include <iostream>
#include <iomanip>

GestorBuffer::GestorBuffer(GestorDisco& disco, size_t numMarcos, TipoPolitica tipo)
    : disco(disco), marcos(numMarcos), tipoPolitica(tipo) {
    for (size_t i = 0; i < numMarcos; ++i) marcos[i].idMarco = static_cast<int>(i);

    switch (tipo) {
        case TipoPolitica::LRU:
            politica = std::make_unique<PoliticaLRU>();
            break;
        case TipoPolitica::MRU:
            politica = std::make_unique<PoliticaMRU>();
            break;
        case TipoPolitica::Reloj:
            politica = std::make_unique<PoliticaReloj>(numMarcos);
            break;
    }
}

GestorBuffer::~GestorBuffer() { volcarTodo(); }

int GestorBuffer::buscarMarcoVacio() {
    for (size_t i = 0; i < marcos.size(); ++i) {
        if (marcos[i].estaVacio()) return static_cast<int>(i);
    }
    return -1;
}

Marco* GestorBuffer::fijarPagina(int idPagina) {
    auto it = tablaPaginas.find(idPagina);
    if (it != tablaPaginas.end()) {
        int idMarco = it->second;
        marcos[idMarco].contadorFijacion++;
        marcos[idMarco].ultimoUso = ++relojLogico;
        marcos[idMarco].bitReferencia = true;
        politica->alAcceder(idMarco);
        hits++;
        return &marcos[idMarco];
    }

    misses++;

    int idMarco = buscarMarcoVacio();

    if (idMarco == -1) {
        idMarco = politica->seleccionarVictima();
        if (idMarco == -1) {
            std::cerr << "ERROR: Todos los marcos estan fijados, no hay victima.\n";
            return nullptr;
        }
        Marco& victima = marcos[idMarco];
        if (victima.bitSucio) {
            disco.escribirBloque(victima.bloque);
        }
        tablaPaginas.erase(victima.idPagina);
    }

    Marco& m = marcos[idMarco];
    if (!disco.leerBloque(idPagina, m.bloque)) {
        std::cerr << "ERROR: No se pudo leer la pagina " << idPagina
                  << " del disco.\n";
        m.idPagina = -1;
        return nullptr;
    }
    m.idPagina = idPagina;
    m.bitSucio = false;
    m.contadorFijacion = 1;
    m.ultimoUso = ++relojLogico;
    m.bitReferencia = true;
    tablaPaginas[idPagina] = idMarco;
    politica->alCargar(idMarco);
    politica->alAcceder(idMarco);
    return &m;
}

void GestorBuffer::liberarPagina(int idPagina, bool sucio) {
    auto it = tablaPaginas.find(idPagina);
    if (it == tablaPaginas.end()) {
        std::cerr << "ADVERTENCIA: liberar pagina " << idPagina
                  << " que no esta en buffer.\n";
        return;
    }
    Marco& m = marcos[it->second];
    if (sucio) m.bitSucio = true;
    if (m.contadorFijacion > 0) m.contadorFijacion--;
    if (m.contadorFijacion == 0) politica->alLiberar(it->second);
}

void GestorBuffer::volcarTodo() {
    for (auto& m : marcos) {
        if (!m.estaVacio() && m.bitSucio) {
            disco.escribirBloque(m.bloque);
            m.bitSucio = false;
        }
    }
}

double GestorBuffer::tasaAciertos() const {
    size_t total = hits + misses;
    return total == 0 ? 0.0 : static_cast<double>(hits) / total;
}

void GestorBuffer::imprimirEstado(const std::string& etiqueta) const {
    if (!etiqueta.empty()) std::cout << ">> " << etiqueta << "\n";

    const bool esReloj = (tipoPolitica == TipoPolitica::Reloj);
    const auto* reloj = esReloj ? static_cast<const PoliticaReloj*>(politica.get()) : nullptr;

    std::cout << "+-------+--------+-------+----------+-----------+----------+\n";
    std::cout << "| Marco | Pagina | Sucio | Fijacion | UltimoUso | BitRef   |\n";
    std::cout << "+-------+--------+-------+----------+-----------+----------+\n";
    for (const auto& m : marcos) {
        bool ref = reloj ? reloj->bitReferencia(m.idMarco) : m.bitReferencia;
        std::cout << "| " << std::setw(5) << m.idMarco << " | "
                  << std::setw(6) << (m.estaVacio() ? -1 : m.idPagina) << " | "
                  << std::setw(5) << (m.bitSucio ? 1 : 0) << " | "
                  << std::setw(8) << m.contadorFijacion << " | "
                  << std::setw(9) << m.ultimoUso << " | "
                  << std::setw(8) << (ref ? 1 : 0) << " |\n";
    }
    std::cout << "+-------+--------+-------+----------+-----------+----------+\n";
    if (reloj) {
        std::cout << "Manecilla del reloj -> marco " << reloj->posicionManecilla() << "\n";
    }
    std::cout << "Politica: " << politica->nombre()
              << " | Hits: " << hits << " | Misses: " << misses
              << " | Hit rate: " << std::fixed << std::setprecision(2)
              << (tasaAciertos() * 100.0) << "%\n\n";
}
