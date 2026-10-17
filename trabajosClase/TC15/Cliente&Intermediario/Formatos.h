#ifndef FORMATOS_H
#define FORMATOS_H

#include <cstdint>

const int TAM_BLOQUE = 256;
const int NUM_BLOQUES_CAFETERIA = 1024;
const int BITMAP_BYTES = NUM_BLOQUES_CAFETERIA / 8; // 128

const int TAM_REGISTRO = 32;
const int PRODUCTOS_POR_BLOQUE = TAM_BLOQUE / TAM_REGISTRO;      // 8
const int CASILLAS_POR_BLOQUE_DIR = PRODUCTOS_POR_BLOQUE - 1;    // 7, el slot 0 es el encabezado
const uint16_t BLOQUE_NULO = 0xFFFF;                              // no hay siguiente bloque

const int MAX_BLOQUES_INDICE = 120;

const char FIRMA_CAFETERIA[8] = "CAFE02";

#pragma pack(push, 1)

// bloque 0: superbloque de cafeteria.dat completo
struct Superbloque {
    char firma[8];
    char nombreNegocio[32];
    uint8_t numBodegas;
    uint16_t bloqueDirBodegas; // primer bloque del directorio de bodegas
    uint16_t bloquesLibres;    // informativo
    char relleno[211];
};

// va al inicio de todo bloque de directorio (bodegas o categorias), ocupa el slot 0
struct EncabezadoDirectorio {
    uint16_t siguienteBloque; // BLOQUE_NULO si no hay mas
    uint8_t casillasUsadas;
    char relleno[29];
};

// casilla de bodega, 32 bytes
struct DirBodega {
    char nombre[16];
    char id[5];
    uint8_t estado; // 0 libre, 1 ocupada
    uint16_t bloqueDirCategorias; // donde arranca el directorio de categorias de esta bodega
    char relleno[8];
};

// casilla de categoria, 32 bytes
struct DirCategoria {
    char nombre[16];
    char id[6];
    uint8_t estado;
    uint16_t bloqueIndice;
    char relleno[7];
};

// un bloque completo de directorio de bodegas: encabezado + 7 casillas = 256 bytes
struct BloqueDirBodegas {
    EncabezadoDirectorio encabezado;
    DirBodega casillas[CASILLAS_POR_BLOQUE_DIR];
};

// un bloque completo de directorio de categorias: encabezado + 7 casillas = 256 bytes
struct BloqueDirCategorias {
    EncabezadoDirectorio encabezado;
    DirCategoria casillas[CASILLAS_POR_BLOQUE_DIR];
};

// bloque de indice de una categoria
struct BloqueIndice {
    uint16_t bloquesDatos[MAX_BLOQUES_INDICE];
    uint8_t cantidadBloques;
    uint16_t cantidadProductos;
    char relleno[13];
};

// registro de producto, 32 bytes
struct RegistroProducto {
    char nombre[24];
    uint16_t cantidad;
    uint32_t precioCentavos;
    uint8_t estado;
    char relleno[1];
};

#pragma pack(pop)

#endif