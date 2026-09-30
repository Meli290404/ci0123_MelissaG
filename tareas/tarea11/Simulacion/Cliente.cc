#include <thread>
#include <chrono>
#include <vector>

#include "Cliente.h"
#include "Bitacora.h"
#include "Protocolo.h"

Cliente::Cliente( Buzon & buzon ) : buzon_( buzon ) {
}

void Cliente::pedir( int tipo, const std::vector<std::string> & campos ) {
    std::string peticion = armarMensaje( ID_TXT_CLIENTE, ID_TXT_INTERMEDIARIO, tipo, campos );
    buzon_.Enviar( peticion.c_str(), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, peticion, "solicitud enviada" );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, std::string( resp.texto ), "respuesta recibida por el cliente" );

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
}

void Cliente::ejecutar() {
    // Pausa para dejar que las dos bodegas se anuncien antes de empezar
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

    pedir( MSG_REQ_CAT_CLI, { "BebidasCalientes" } );  // caso normal (Bodega1)
    pedir( MSG_REQ_CAT_CLI, { "co" } );                // ERR_FORMAT: categoria muy corta
    pedir( MSG_REQ_CAT_CLI, { "Licores" } );            // formato valido, ninguna bodega la atiende
    pedir( MSG_REQ_CAT_CLI, { "Caramelos" } );          // caso normal (Bodega2)

    pedir( MSG_REQ_PROD_CLI, { "capuchino" } );         // existe en Bodega1
    pedir( MSG_REQ_PROD_CLI, { "menta" } );              // existe en Bodega2
    pedir( MSG_REQ_PROD_CLI, { "torta#3" } );            // ERR_FORMAT: caracter invalido
    pedir( MSG_REQ_PROD_CLI, { "flandecoco" } );         // formato valido, no existe en ninguna bodega
    pedir( MSG_REQ_PROD_CLI, { "silenciar" } );          // Bodega1 no responde -> ERR_COMM

    pedir( MSG_REQUEST_FACTURA, { "0" } );               // carrito vacio antes de agregar nada
    pedir( MSG_ADD_CART, { "capuchino", "3" } );         // agrega producto valido
    pedir( MSG_ADD_CART, { "menta", "5" } );             // agrega otro producto (otra bodega)
    pedir( MSG_ADD_CART, { "tarta", "999" } );           // ERR_STOCK: pide mas de lo que hay
    pedir( MSG_ADD_CART, { "torta#3", "2" } );           // ERR_FORMAT: nombre no cumple regex
    pedir( MSG_ADD_CART, { "flan", "abc" } );            // ERR_FORMAT: count no cumple regex
    pedir( MSG_REQUEST_FACTURA, { "2", "capuchino,3;menta,5" } ); // cierra el pedido

    std::string death = armarMensaje( ID_TXT_CLIENTE, ID_TXT_INTERMEDIARIO, MSG_DEATH, {} );
    buzon_.Enviar( death.c_str(), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, death, "cliente termino su guion, avisa cierre" );
}