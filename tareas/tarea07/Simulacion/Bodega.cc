#include <sstream>
#include <iomanip>
#include <algorithm>

#include "Bodega.h"
#include "Bitacora.h"

static std::string formatearPrecio( float precio ) {
    std::ostringstream out;
    out << std::fixed << std::setprecision( 2 ) << precio;
    return out.str();
}

Bodega::Bodega( Buzon & buzon, int miId, const std::string & nombre,
                 std::vector<std::string> categorias, std::vector<Producto> productos )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ),
      categorias_( std::move( categorias ) ), productos_( std::move( productos ) ) {
}

// Formato ANNOUNCE: SERV|INT_04/60/<id_interno>/<nombre>/<cat1,cat2,...>
// El id_interno hace falta porque el protocolo exige "SERV" literal como
// ORIGEN, asi que el intermediario no puede distinguir las dos bodegas
// solo con ese campo.
void Bodega::anunciarse() {
    std::ostringstream cats;
    for ( size_t i = 0; i < categorias_.size(); ++i ) {
        cats << categorias_[ i ];
        if ( i + 1 < categorias_.size() ) cats << ",";
    }

    std::string msg = armarMensaje( ID_TXT_BODEGA, ID_TXT_INTERMEDIARIO, MSG_ANNOUNCE,
                                     { std::to_string( miId_ ), nombre_, cats.str() } );
    buzon_.Enviar( msg.c_str(), CANAL_REGISTRO );
    rotular( miId_, ID_INTERMEDIARIO, msg, "la bodega se anuncia" );
}

// PROD_LIST con count 0 si la categoria no es mia; si es mia, listado de
// solo los productos de esa categoria (ya no de todo el inventario).
void Bodega::atenderListProd( const std::string & categoria ) {
    bool laAtiendo = std::find( categorias_.begin(), categorias_.end(), categoria ) != categorias_.end();

    if ( !laAtiendo ) {
        std::string msg = armarMensaje( ID_TXT_BODEGA, ID_TXT_INTERMEDIARIO, MSG_PROD_LIST, { "0" } );
        buzon_.Enviar( msg.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, msg, "categoria que no me corresponde" );
        return;
    }

    std::ostringstream lista;
    int cantidad = 0;
    for ( const auto & p : productos_ ) {
        if ( p.categoria != categoria ) continue;
        lista << p.nombre << "," << formatearPrecio( p.precio ) << "," << p.stock << ";";
        ++cantidad;
    }

    std::string msg = armarMensaje( ID_TXT_BODEGA, ID_TXT_INTERMEDIARIO, MSG_PROD_LIST,
                                     { std::to_string( cantidad ), lista.str() } );
    buzon_.Enviar( msg.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, msg, "listado de la categoria " + categoria );
}

void Bodega::atenderFindProd( const std::string & nombre ) {
    if ( nombre == "silenciar" ) {
        // Caso de prueba: esta bodega se cae y no responde, para
        // poder demostrar ERR_COMM en el intermediario.
        rotular( ID_INTERMEDIARIO, miId_, "(pedido de " + nombre + ")",
                  "(" + nombre_ + " no va a responder: simulando falla)" );
        return;
    }

    auto it = std::find_if( productos_.begin(), productos_.end(),
                             [ & ]( const Producto & p ) { return p.nombre == nombre; } );

    if ( it != productos_.end() ) {
        std::string msg = armarMensaje( ID_TXT_BODEGA, ID_TXT_INTERMEDIARIO, MSG_PROD_FOUND,
                                         { it->nombre, formatearPrecio( it->precio ),
                                           std::to_string( it->stock ), it->descripcion } );
        buzon_.Enviar( msg.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, msg, "producto encontrado" );
    } else {
        std::string msg = armarMensaje( ID_TXT_BODEGA, ID_TXT_INTERMEDIARIO, MSG_PROD_NOT_FOUND, { nombre } );
        buzon_.Enviar( msg.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, msg, "no esta en esta bodega" );
    }
}

void Bodega::ejecutar() {
    anunciarse();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_INTERMEDIARIO, miId_ ) );
        } catch ( const std::runtime_error & ) {
            return;
        }

        std::string linea( msg.texto );
        MensajeProtocolo mp;
        try {
            mp = parsearMensaje( linea );
        } catch ( const std::invalid_argument & ) {
            continue; // mensaje mal formado; en esta simulacion no debería pasar
        }

        if ( mp.tipo == MSG_DEATH ) {
            rotular( ID_INTERMEDIARIO, miId_, linea, "orden de cierre: " + nombre_ + " termina su hilo" );
            return;
        } else if ( mp.tipo == MSG_REQ_CAT_BOD && !mp.campos.empty() ) {
            atenderListProd( mp.campos[ 0 ] );
        } else if ( mp.tipo == MSG_REQ_PROD_BOD && !mp.campos.empty() ) {
            atenderFindProd( mp.campos[ 0 ] );
        }
    }
}