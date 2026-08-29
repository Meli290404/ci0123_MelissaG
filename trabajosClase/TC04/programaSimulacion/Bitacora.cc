#include <cstdio>

#include "Bitacora.h"

void rotular( const std::string & origen, const std::string & destino,
              const std::string & linea, const std::string & explicacion ) {
    printf( "[%-13s -> %-13s] %-45s %s\n",
            origen.c_str(), destino.c_str(), linea.c_str(), explicacion.c_str() );
}