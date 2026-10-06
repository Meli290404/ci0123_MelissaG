#include <thread>
#include <chrono>
#include <vector>

#include "Cliente.h"
#include "Bitacora.h"
#include "Protocolo.h"

Cliente::Cliente( Buzon & buzon ) : buzon_( buzon ) {
}

void Cliente::pedir( int tipo, const std::vector<std::string> & campos ) {
    std::string peticion = armarMensaje( ID_TXT_CLIENTE, ID_TXT_INT04, tipo, campos );
    rotular( ID_CLIENTE, ID_INT04, peticion, "solicitud enviada" );
    buzon_.Enviar( peticion.c_str(), canal( ID_CLIENTE, ID_INT04 ) );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( ID_INT04, ID_CLIENTE ) );
    rotular( ID_INT04, ID_CLIENTE, std::string( resp.texto ), "respuesta recibida por el cliente" );

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
}

void Cliente::ejecutar() {
    // escenario 1: categorias de la isla 4 y de la isla 5
    pedir( MSG_REQ_CATEGORIES, {} );

    // escenarios 2 y 3: productos de una categoria
    pedir( MSG_REQ_PRODUCTS, { "BebidasCalientes" } );   // Bodega1 de la isla 4
    pedir( MSG_REQ_PRODUCTS, { "helados" } );            // isla 5: el intermediario usa el mancomunado
    pedir( MSG_REQ_PRODUCTS, { "BebidasFrias" } );       // 23: Bodega1 la atiende pero no tiene productos
    pedir( MSG_REQ_PRODUCTS, { "Licores" } );            // 23: ninguna isla la atiende
    pedir( MSG_REQ_PRODUCTS, { "co" } );                 // 90: categoria muy corta

    // escenario 7: factura con el carrito vacio
    pedir( MSG_REQUEST_FACTURA, { "0" } );

    // escenarios 4 y 5: agregar al carrito en la isla 4
    pedir( MSG_ADD_CART, { "capuchino", "3" } );         // 32: reserva completa en Bodega1
    pedir( MSG_ADD_CART, { "menta", "5" } );             // Bodega1 responde 27, se reserva en Bodega2
    pedir( MSG_ADD_CART, { "tarta", "999" } );           // 32 parcial: solo hay 10
    pedir( MSG_ADD_CART, { "tarta", "1" } );             // 32 con 0: ya no queda stock

    // agregar al carrito en la isla 5 con el mancomunado
    pedir( MSG_ADD_CART, { "cono", "2" } );              // ya se vio en el listado de helados: 50 y 01
    pedir( MSG_ADD_CART, { "cono", "999" } );            // 02/103 de la isla 5: 32 con 0
    pedir( MSG_ADD_CART, { "papas", "1" } );             // no se ha listado: se busca con 20 en la isla 5
    pedir( MSG_ADD_CART, { "flandecoco", "1" } );        // 28: no existe en ninguna isla

    // escenarios 8, 9 y 10: errores
    pedir( MSG_ADD_CART, { "torta#3", "2" } );           // 90: producto no cumple regex
    pedir( MSG_ADD_CART, { "flan", "abc" } );            // 90: cantidad no cumple regex
    pedir( MSG_ADD_CART, { std::string( 60, 'a' ), "1" } ); // 91: producto de 60 caracteres, maximo 50
    pedir( MSG_ADD_CART, { "silenciar", "1" } );         // 92: Bodega1 no responde

    // escenario 6: factura con lo reservado en las dos islas
    pedir( MSG_REQUEST_FACTURA, { "5", "capuchino,3;menta,5;tarta,10;cono,2;papas,1" } );

    std::string death = armarMensaje( ID_TXT_CLIENTE, ID_TXT_INT04, MSG_DEATH, {} );
    rotular( ID_CLIENTE, ID_INT04, death, "cliente termino su guion, avisa cierre" );
    buzon_.Enviar( death.c_str(), canal( ID_CLIENTE, ID_INT04 ) );
}
