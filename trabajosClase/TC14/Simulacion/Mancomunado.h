#ifndef MANCOMUNADO_H
#define MANCOMUNADO_H

#include <string>
#include <vector>

#include "Protocolo.h"

// Protocolo mancomunado entre islas: TIPO_MENSAJE/campo1/campo2/...
// No lleva origen ni destino porque en la red real los da el socket.
const int PUERTO_MANCOMUNADO = 9090;

const int MAN_ACK            = 1;   // respuesta exitosa con los datos pedidos
const int MAN_ERROR          = 2;   // 02/codigo
const int MAN_NOT_FOUND      = 3;   // 03/valor: la categoria o el producto no existe en la isla
const int MAN_GET_CATEGORIES = 10;
const int MAN_GET_PRODUCTS   = 20;  // 20/categoria
const int MAN_BUY            = 40;  // 40/n/producto,cantidad;...
const int MAN_RESERVE        = 50;  // 50/producto/cantidad
const int MAN_ANNOUNCE       = 60;  // 60/ID_MSG/id_isla/ip/puerto, por broadcast
const int MAN_ANNOUNCE_BYE   = 61;  // 61/id_isla, por broadcast

// codigos que acompanan a MAN_ERROR
const int ERR_MAL_FORMADO       = 100;
const int ERR_TIPO_NO_SOPORTADO = 101;
const int ERR_VALOR_INVALIDO    = 102;
const int ERR_STOCK             = 103;
const int ERR_SIN_RESERVA       = 104;

const int SEGUNDOS_VIGENCIA_RESERVA = 60;

// canales del buzon que hacen el papel de la red entre islas
inline long canalPuerto( int idIsla ) {      // puerto 9090 de la isla: anuncios y solicitudes
    return 1000 + idIsla;
}
inline long canalRespuesta( int idIsla ) {   // respuestas a las solicitudes que hace esa isla
    return 2000 + idIsla;
}

// Lo que viaja por el buzon entre intermediarios. canalRespuesta e idOrigen
// hacen el papel de la conexion tcp (ip y puerto de quien envia); en un
// anuncio por broadcast canalRespuesta va en 0 porque no se responde.
// texto es el mensaje del protocolo tal como se enviaria por la red.
struct DatosRed {
    long canalRespuesta;
    int idOrigen;
    char texto[ TAM_MAX_MENSAJE ];
};

struct PaqueteRed {
    long mtype;
    DatosRed datos;
};

struct MensajeMancomunado {
    int tipo;
    std::vector<std::string> campos;
};

// Arma una linea TIPO/campo1/campo2/... con el tipo en dos digitos
std::string armarMancomunado( int tipo, const std::vector<std::string> & campos );

// Separa una linea recibida. Si el tipo no son dos digitos lanza std::invalid_argument,
// que el receptor responde con 02/100.
MensajeMancomunado parsearMancomunado( const std::string & linea );

#endif
