#ifndef PROTOCOLO_H
#define PROTOCOLO_H

// Canales para saber a quien va dirigido cada mensaje dentro de la misma cola.
const long CANAL_A_BODEGA        = 1;
const long CANAL_A_INTERMEDIARIO = 2;

const int TAM_MAX_MENSAJE = 256;

#endif