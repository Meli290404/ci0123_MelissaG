// Simula el servidor de productos (bodega).
// Se queda esperando peticiones del intermediario y responde segun el catalogo.

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bitacora.h"

struct Producto {
    std::string nombre;
    float precio;
    int stock;
    std::string descripcion;
};

int main() {
    Buzon buzon;

    std::vector<Producto> catalogo = {
        { "pan",    1000.0f, 10, "pan artesanal" },
        { "leche",   900.0f,  5, "leche entera 1L" },
        { "azucar",  500.0f, 20, "azucar blanca 1kg" }
    };

    printf( "Bodega lista, esperando peticiones del intermediario...\n" );

    while ( true ) {
        // se bloquea aqui hasta que llegue algo dirigido a la bodega
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;
        buzon.Recibir( &msg, sizeof( msg.texto ), CANAL_A_BODEGA );

        std::string linea( msg.texto );
        rotular( "Intermediario", "Bodega", linea, "peticion recibida" );

        // se separa el primer token para saber que verbo es
        std::istringstream iss( linea );
        std::string verbo;
        iss >> verbo;

        if ( verbo == "EXIT" ) {
            rotular( "Intermediario", "Bodega", linea, "orden de cierre: la bodega termina" );
            break;
        } else if ( verbo == "LIST_PROD" ) {
            // arma la lista completa como nombre,precio,stock; separados por ;
            std::ostringstream resp;
            resp << "PROD_LIST " << catalogo.size() << " ";
            for ( const auto & p : catalogo ) {
                resp << p.nombre << "," << p.precio << "," << p.stock << ";";
            }
            buzon.Enviar( resp.str().c_str(), CANAL_A_INTERMEDIARIO );
            rotular( "Bodega", "Intermediario", resp.str(), "listado de productos" );
        } else if ( verbo == "FIND_PROD" ) {
            // busca un producto especifico por nombre
            std::string nombre;
            iss >> nombre;

            auto it = std::find_if( catalogo.begin(), catalogo.end(),
                                     [ & ]( const Producto & p ) { return p.nombre == nombre; } );

            if ( it != catalogo.end() ) {
                std::ostringstream resp;
                resp << "PROD_FOUND " << it->nombre << " " << it->precio << " "
                     << it->stock << " " << it->descripcion;
                buzon.Enviar( resp.str().c_str(), CANAL_A_INTERMEDIARIO );
                rotular( "Bodega", "Intermediario", resp.str(), "producto encontrado" );
            } else {
                std::ostringstream resp;
                resp << "PROD_NOT_FOUND " << nombre;
                buzon.Enviar( resp.str().c_str(), CANAL_A_INTERMEDIARIO );
                rotular( "Bodega", "Intermediario", resp.str(), "producto no existe" );
            }
        }
    }

    return 0;
}