#include <cstdio>
#include <mutex>

#include "Bitacora.h"
#include "Protocolo.h"

static std::mutex mtxPantalla;

static const char * nombreDe( int id ) {
    switch ( id ) {
        case ID_CLIENTE:   return "Cliente";
        case ID_INT04:     return "INT_04";
        case ID_BODEGA1:   return "Bodega1";
        case ID_BODEGA2:   return "Bodega2";
        case ID_INT05:     return "INT_05";
        case ID_BODEGA3:   return "Bodega3";
        case ID_BROADCAST: return "Broadcast";
        default:           return "?";
    }
}

void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion ) {
    std::lock_guard<std::mutex> guard( mtxPantalla );
    printf( "[%-9s -> %-9s] %-55s %s\n",
            nombreDe( origen ), nombreDe( destino ), linea.c_str(), explicacion.c_str() );
}

void titulo( const std::string & texto ) {
    std::lock_guard<std::mutex> guard( mtxPantalla );
    printf( "\n========== %s ==========\n", texto.c_str() );
}
