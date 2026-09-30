#include "Bitmap.h"

Bitmap::Bitmap(std::fstream& archivo) : archivo_(archivo) {}

void Bitmap::leer(uint8_t mapa[BITMAP_BYTES]) {
    archivo_.seekg(BLOQUE_BITMAP * TAM_BLOQUE);
    archivo_.read(reinterpret_cast<char*>(mapa), BITMAP_BYTES);
}

void Bitmap::escribir(uint8_t mapa[BITMAP_BYTES]) {
    archivo_.seekp(BLOQUE_BITMAP * TAM_BLOQUE);
    archivo_.write(reinterpret_cast<char*>(mapa), BITMAP_BYTES);
    archivo_.flush();
}

void Bitmap::inicializar(int bloquesReservados) {
    uint8_t mapa[BITMAP_BYTES] = {0};
    for (int b = 0; b < bloquesReservados; b++) {
        mapa[b / 8] |= (1 << (7 - (b % 8)));
    }
    escribir(mapa);
}

int Bitmap::pedirBloque() {
    uint8_t mapa[BITMAP_BYTES];
    leer(mapa);

    for (int bloque = 0; bloque < NUM_BLOQUES_CAFETERIA; bloque++) {
        int byteIdx = bloque / 8;
        int bitIdx = 7 - (bloque % 8);
        if (!(mapa[byteIdx] & (1 << bitIdx))) {
            mapa[byteIdx] |= (1 << bitIdx);
            escribir(mapa);
            return bloque;
        }
    }
    return -1;
}

void Bitmap::liberarBloque(int numBloque) {
    uint8_t mapa[BITMAP_BYTES];
    leer(mapa);
    mapa[numBloque / 8] &= ~(1 << (7 - (numBloque % 8)));
    escribir(mapa);
}

int Bitmap::bloquesLibres() {
    uint8_t mapa[BITMAP_BYTES];
    leer(mapa);
    int libres = 0;
    for (int bloque = 0; bloque < NUM_BLOQUES_CAFETERIA; bloque++) {
        if (!(mapa[bloque / 8] & (1 << (7 - (bloque % 8))))) libres++;
    }
    return libres;
}