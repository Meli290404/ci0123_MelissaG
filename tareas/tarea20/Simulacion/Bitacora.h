#ifndef BITACORA_H
#define BITACORA_H

#include <string>

// Imprime una línea con el formato [origen -> destino] mensaje y explicación,
// Internamente usa un mutex para que los rótulos de los distintos hilos no se mezclen.
void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion );

// Imprime un acceso de un hilo al filesystem de su isla: quien, que archivo y que hizo
void rotularDisco( int quien, const std::string & archivo, const std::string & accion );

// Imprime un separador con el nombre de la etapa de la simulacion
void titulo( const std::string & texto );

#endif
