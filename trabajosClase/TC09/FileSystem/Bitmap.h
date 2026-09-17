#ifndef BITMAP_H
#define BITMAP_H

#include <fstream>
#include <cstdint>
#include "Formatos.h"

// maneja el bloque 1 de cafeteria.dat: 1024 bits, uno por cada bloque del archivo
class Bitmap {
public:
    Bitmap(std::fstream& archivo);

    void inicializar(int bloquesReservados); // marca 0..bloquesReservados-1 como ocupados
    int pedirBloque();
    void liberarBloque(int numBloque);
    int bloquesLibres();

private:
    std::fstream& archivo_;
    static const int BLOQUE_BITMAP = 1;

    void leer(uint8_t mapa[BITMAP_BYTES]);
    void escribir(uint8_t mapa[BITMAP_BYTES]);
};

#endif