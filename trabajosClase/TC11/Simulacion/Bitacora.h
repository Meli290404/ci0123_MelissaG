#ifndef BITACORA_H
#define BITACORA_H

#include <string>

// Imprime una línea con el formato [origen -> destino] mensaje y explicación,
// Internamente usa un mutex para que los rótulos de los distintos hilos no se mezclen.
void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion );

#endif