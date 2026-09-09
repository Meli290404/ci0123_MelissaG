#include <thread>
#include <chrono>
#include <vector>
#include <sstream>

#include "Cliente.h"
#include "Bitacora.h"
#include "Protocolo.h"

Cliente::Cliente( Buzon & buzon ) : buzon_( buzon ) {
}

void Cliente::pedir( const std::string & peticion ) {
    buzon_.Enviar( peticion.c_str(), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, peticion, "solicitud enviada" );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, std::string( resp.texto ), "respuesta recibida por el cliente" );

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) ); // pausa para que el log sea legible
}

void Cliente::ejecutar() {
    // Pausa para evitar que el primer HOLA de la bodega se mezcle con el del cliente
    std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );

    std::vector<std::string> peticiones = {
        "RE_CAT Bebidas",       // caso normal: categoria de Bodega
        "RE_CAT co",            // ERR_FORMAT: categoria muy corta
        "RE_CAT Licores",       // formato valido, pero ninguna bodega la atiende
        "RE_CAT Caramelos",     // caso normal: categoria de Bodega
        "RE_PROD capuchino",    // existe en Bodega
        "RE_PROD menta",        // existe, en la categoria Caramelos 
        "RE_PROD torta#3",      // ERR_FORMAT: caracter invalido
        "RE_PROD flan-de-coco",// formato valido, pero no existe en bodega
        "RE_PROD silenciar"     // Bodega no responde -> ERR_COMM

        // casos de prueba factura y carrito
        "GET_FACT 0",                     // carrito vacio antes de agregar nada
        "ADD_CART capuchino 3",           // agrega producto valido
        "ADD_CART menta 5",               // agrega otro producto
        "ADD_CART tarta 999",             // ERR_STOCK: pide mas de lo que hay (stock 10)
        "ADD_CART torta#3 2",             // ERR_FORMAT: nombre no cumple regex
        "ADD_CART flan abc",              // ERR_FORMAT: count no cumple regex
        "GET_FACT 2 capuchino,3;menta,5"  // cierra el pedido con lo confirmado antes
    };

    for ( const auto & peticion : peticiones ) {
        pedir( peticion );
    }

    // Fin del guion de pruebas. Avisa a intermediario y este avisa a la bodega que ya puede terminar su ciclo.
    buzon_.Enviar( "EXIT", canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, "EXIT", "cliente termino su guion, avisa cierre" );
}