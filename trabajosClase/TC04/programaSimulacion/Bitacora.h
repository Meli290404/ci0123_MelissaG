#ifndef BITACORA_H
#define BITACORA_H

#include <string>

// Imprime una linea mostrando quien le mando que a quien.
void rotular( const std::string & origen, const std::string & destino,
              const std::string & linea, const std::string & explicacion );

#endif