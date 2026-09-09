#include <sstream>
#include <algorithm>

#include "Bodega.h"
#include "Bitacora.h"

Bodega::Bodega( Buzon & buzon, int miId, const std::string & nombre,
                 std::vector<std::string> categorias, std::vector<Producto> productos )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ),
      categorias_( std::move( categorias ) ), productos_( std::move( productos ) ) {
}

// Se anuncia ante el intermediario con su ID y las categorías que atiende.
// Formato: HOLA <id> <nombre> <cat1,cat2,...>
void Bodega::anunciarse() {
    std::ostringstream msg;
    msg << "HOLA " << miId_ << " " << nombre_ << " ";
    for ( size_t i = 0; i < categorias_.size(); ++i ) {
        msg << categorias_[ i ];
        if ( i + 1 < categorias_.size() ) msg << ",";
    }
    buzon_.Enviar( msg.str().c_str(), CANAL_REGISTRO );
    rotular( miId_, ID_INTERMEDIARIO, msg.str(), "la bodega se anuncia" );
}

// Atiende un pedido de listado de productos de una categoría.
void Bodega::atenderListProd( const std::string & categoria ) {
    bool laAtiendo = std::find( categorias_.begin(), categorias_.end(), categoria ) != categorias_.end();

    if ( !laAtiendo ) {
        // Si la lista está vacía
        buzon_.Enviar( "PROD_LIST 0", canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, "PROD_LIST 0", "categoria que no me corresponde" );
        return;
    }

    std::ostringstream resp;
    int cantidad = 0;
    std::ostringstream lista;
    for ( const auto & p : productos_ ) {
        // Cada producto "pertenece" a una sola categoría de esta bodega.
        lista << p.nombre << "," << p.precio << "," << p.stock << ";";
        ++cantidad;
    }
    resp << "PROD_LIST " << cantidad << " " << lista.str();
    buzon_.Enviar( resp.str().c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp.str(), "listado de la categoria " + categoria );
}

void Bodega::atenderFindProd( const std::string & nombre ) {
    if ( nombre == "silenciar" ) {
        // Caso de prueba, esta bodega se cae y no responde,
        // para poder demostrar ERR_COMM en el intermediario.
        rotular( ID_INTERMEDIARIO, miId_, "FIND_PROD " + nombre, "(" + nombre_ + " no va a responder: simulando falla)" );
        return;
    }

    auto it = std::find_if( productos_.begin(), productos_.end(),
                             [ & ]( const Producto & p ) { return p.nombre == nombre; } );

    if ( it != productos_.end() ) {
        std::ostringstream resp;
        resp << "PROD_FOUND " << it->nombre << " " << it->precio << " " << it->stock << " " << it->descripcion;
        buzon_.Enviar( resp.str().c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, resp.str(), "producto encontrado" );
    } else {
        std::ostringstream resp;
        resp << "PROD_NOT_FOUND " << nombre;
        buzon_.Enviar( resp.str().c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, resp.str(), "no esta en esta bodega" );
    }
}

void Bodega::ejecutar() {
    anunciarse();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_INTERMEDIARIO, miId_ ) );
        } catch ( const std::runtime_error & ) {
            return; // el buzon fue destruido: salida de respaldo
        }

        std::string linea( msg.texto );
        std::istringstream iss( linea );
        std::string verbo;
        iss >> verbo;

        if ( verbo == "EXIT" ) {
            rotular( ID_INTERMEDIARIO, miId_, linea, "orden de cierre: " + nombre_ + " termina su hilo" );
            return;
        } else if ( verbo == "LIST_PROD" ) {
            std::string categoria;
            iss >> categoria;
            atenderListProd( categoria );
        } else if ( verbo == "FIND_PROD" ) {
            std::string nombre;
            iss >> nombre;
            atenderFindProd( nombre );
        }
    }
}