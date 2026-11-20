#include "GestorBuffer.hpp"
#include "GestorDisco.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>

static std::vector<int> parsearPatron(const std::string& s) {
    std::vector<int> out;
    std::stringstream ss(s);
    std::string tok;
    while (ss >> tok) {
        if (!tok.empty() && tok.back() == ',') tok.pop_back();
        if (tok.empty()) continue;
        if (std::isalpha(static_cast<unsigned char>(tok[0]))) {
            out.push_back(std::toupper(static_cast<unsigned char>(tok[0])) - 'A');
        } else {
            try { out.push_back(std::stoi(tok)); } catch (...) {}
        }
    }
    return out;
}

static void simular(GestorDisco& disco, size_t numMarcos, TipoPolitica tipo,
                    const std::vector<int>& patron, bool trazarPaso) {
    GestorBuffer buffer(disco, numMarcos, tipo);
    std::cout << "============================================================\n";
    std::cout << " Simulacion: politica=" << buffer.nombrePolitica()
              << ", marcos=" << numMarcos
              << ", solicitudes=" << patron.size() << "\n";
    std::cout << "============================================================\n";

    for (size_t i = 0; i < patron.size(); ++i) {
        int idPagina = patron[i];
        std::cout << "[" << (i + 1) << "] Solicita pagina " << idPagina << "\n";
        Marco* m = buffer.fijarPagina(idPagina);
        if (m) buffer.liberarPagina(idPagina, false);
        if (trazarPaso) buffer.imprimirEstado();
    }

    if (!trazarPaso) buffer.imprimirEstado("Estado final");
    std::cout << "RESULTADO (" << buffer.nombrePolitica() << "): "
              << buffer.aciertos() << " hits / "
              << buffer.fallos() << " misses, hit rate = "
              << (buffer.tasaAciertos() * 100.0) << "%\n\n";
}

int main(int argc, char** argv) {
    std::string rutaBin = "datos/ratings.bin";
    size_t numMarcos = 4;
    std::string patronStr = "A B D D E A E C A B C A E";
    bool trazarPaso = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--bin" && i + 1 < argc) rutaBin = argv[++i];
        else if (a == "--marcos" && i + 1 < argc) numMarcos = std::stoul(argv[++i]);
        else if (a == "--patron" && i + 1 < argc) patronStr = argv[++i];
        else if (a == "--trazar") trazarPaso = true;
        else if (a == "--help" || a == "-h") {
            std::cout << "Uso: " << argv[0]
                      << " [--bin ruta] [--marcos N] [--patron \"A B C ...\"] [--trazar]\n"
                      << "Patron: letras (A=0,B=1,...) o enteros separados por espacios.\n";
            return 0;
        }
    }

    GestorDisco disco(rutaBin);
    if (!disco.abierto()) {
        std::cerr << "No se pudo abrir " << rutaBin
                  << ". Ejecuta 'bin/tsv_a_bin' primero.\n";
        return 1;
    }
    std::cout << "Disco: '" << rutaBin << "' con " << disco.numBloques()
              << " bloques de " << TAMANIO_BLOQUE << " bytes.\n\n";

    auto patron = parsearPatron(patronStr);
    if (patron.empty()) {
        std::cerr << "Patron vacio o invalido.\n";
        return 1;
    }

    std::cout << "Patron parseado: ";
    for (int p : patron) std::cout << p << " ";
    std::cout << "\n\n";

    simular(disco, numMarcos, TipoPolitica::LRU,   patron, trazarPaso);
    simular(disco, numMarcos, TipoPolitica::MRU,   patron, trazarPaso);
    simular(disco, numMarcos, TipoPolitica::Reloj, patron, trazarPaso);

    return 0;
}
