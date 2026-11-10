#ifndef FORMATOS_H
#define FORMATOS_H

#include <cstdint>

// estructuras que se escriben tal cual en fs.dat.ci0123
//
// bloque 0      superbloque
// bloque 1      bitmap de bloques libres
// bloque 2      i-nodo del directorio de bodegas
// resto         se pide al bitmap: bloques del directorio, i-nodos de bodegas y bloques de datos
//
// cada bodega es un archivo de texto con un i-nodo propio. su contenido son lineas
//   nombreBodega,categoria,producto,cantidad,precio\n
// por ejemplo
//   Bodega-1,bebidasFrias,CafeFrio,20,1500.00\n

const int TAM_BLOQUE = 256;
const int NUM_BLOQUES_CAFETERIA = 1024;
const int BITMAP_BYTES = NUM_BLOQUES_CAFETERIA / 8; // 128

const uint16_t BLOQUE_NULO = 0xFFFF;   // puntero sin usar

const int BLOQUE_SUPERBLOQUE = 0;
const int BLOQUE_BITMAP = 1;
const int BLOQUE_INODO_DIRECTORIO = 2;
const int BLOQUES_RESERVADOS = 3;      // 0, 1 y 2 nunca se liberan

const char FIRMA_CAFETERIA[8] = "CAFE03";

const uint8_t INODO_DIRECTORIO = 1;
const uint8_t INODO_BODEGA = 2;

const int PUNTEROS_INODO = 122;
const int MAX_TAMANO_ARCHIVO = PUNTEROS_INODO * TAM_BLOQUE;   // 31232 bytes

#pragma pack(push, 1)

// bloque 0
struct Superbloque {
    char firma[8];
    char nombreNegocio[32];
    uint16_t tamBloque;
    uint16_t numBloques;
    uint16_t numBodegas;
    uint16_t inodoDirectorio;  // bloque del i-nodo del directorio de bodegas
    char relleno[208];
};

// un i-nodo ocupa un bloque completo. su numero es el numero de bloque donde esta
struct Inodo {
    uint8_t tipo;                       // INODO_DIRECTORIO o INODO_BODEGA
    uint8_t reservado;
    uint16_t cantidadBloques;           // punteros usados en bloques[]
    uint32_t tamano;                    // bytes validos del archivo
    uint32_t fechaModificacion;         // segundos desde 1970
    uint16_t bloques[PUNTEROS_INODO];   // bloques de datos en orden, BLOQUE_NULO si no se usa
};

// entrada del directorio de bodegas, 32 bytes, 8 por bloque.
// el nombre es la unica llave de la bodega: no se repite y es el mismo que va en sus lineas
struct EntradaDirectorio {
    char nombre[24];
    uint8_t estado;                     // 0 libre, 1 ocupada
    uint8_t reservado;
    uint16_t inodo;                     // bloque del i-nodo de la bodega
    uint32_t fechaCreacion;
};

#pragma pack(pop)

const int ENTRADAS_POR_BLOQUE = TAM_BLOQUE / sizeof(EntradaDirectorio);  // 8

static_assert(sizeof(Superbloque) == TAM_BLOQUE, "el superbloque debe medir un bloque");
static_assert(sizeof(Inodo) == TAM_BLOQUE, "el i-nodo debe medir un bloque");
static_assert(sizeof(EntradaDirectorio) == 32, "la entrada de directorio debe medir 32 bytes");

#endif
