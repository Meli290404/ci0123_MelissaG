#include <cstdio>
#include <mutex>

#include "Bitacora.h"
#include "Protocolo.h"

static std::mutex mtxPantalla;

static const char * nombreDe( int id ) {
    switch ( id ) {
        case ID_CLIENTE:       return "Cliente";
        case ID_INTERMEDIARIO: return "Intermediario";
        case ID_BODEGA:        return "Bodega";
        default:                return "?";
    }
}

void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion ) {
    std::lock_guard<std::mutex> guard( mtxPantalla );
    printf( "[%-13s -> %-13s] %-45s %s\n",
            nombreDe( origen ), nombreDe( destino ), linea.c_str(), explicacion.c_str() );
}