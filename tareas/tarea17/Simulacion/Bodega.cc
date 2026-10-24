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
                 int idIntermediario, const std::string & idTextoIntermediario,
                 std::vector<std::string> categorias, std::vector<Producto> productos )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ),
      idInt_( idIntermediario ), txtInt_( idTextoIntermediario ),
      categorias_( std::move( categorias ) ), productos_( std::move( productos ) ) {
}

std::string Bodega::listaCategorias() const {
    std::string lista;
    for ( const auto & categoria : categorias_ ) {
        if ( !lista.empty() ) lista += ",";
        lista += categoria;
    }
    return lista;
}

// se rotula antes de enviar para que este rotulo salga antes que el del receptor
void Bodega::responder( const std::string & msg, const std::string & explicacion ) {
    rotular( miId_, idInt_, msg, explicacion );
    buzon_.Enviar( msg.c_str(), canal( miId_, idInt_ ) );
}

// Formato del anuncio: SERV|INT_04/60/<id_interno>/<nombre>/<cat1,cat2,...>
// El id_interno hace falta porque ID_TXT_BODEGA es el mismo para todas las
// bodegas, asi que el intermediario no puede distinguirlas solo con el origen.
void Bodega::anunciarse() {
    std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_ANNOUNCE,
                                     { std::to_string( miId_ ), nombre_, listaCategorias() } );
    rotular( miId_, idInt_, msg, "la bodega se anuncia" );
    buzon_.Enviar( msg.c_str(), canalRegistro( idInt_ ) );
}

// Mensaje 10: responde 11 con las categorias de esta bodega.
void Bodega::atenderCategorias() {
    std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_CATEGORY_LIST, { listaCategorias() } );
    responder( msg, "categorias de " + nombre_ );
}

// Mensaje 20: responde 22 con count/lista de la categoria, o 22/0 si esta
// bodega no tiene productos en ella.
void Bodega::atenderProductos( const std::string & categoria ) {
    std::ostringstream lista;
    int cantidad = 0;
    for ( const auto & p : productos_ ) {
        if ( p.categoria != categoria ) continue;
        if ( cantidad > 0 ) lista << ";";
        lista << p.nombre << "," << formatearPrecio( p.precio ) << "," << p.stock;
        ++cantidad;
    }

    if ( cantidad == 0 ) {
        std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_PRODUCT_LIST, { "0" } );
        responder( msg, "no hay productos de " + categoria + " en " + nombre_ );
        return;
    }

    std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_PRODUCT_LIST,
                                     { std::to_string( cantidad ), lista.str() } );
    responder( msg, "listado de la categoria " + categoria );
}

// Mensaje 50: reserva hasta la cantidad pedida, descuenta el stock y responde 51.
// Si no alcanza reserva lo que haya; si el producto no esta en esta bodega responde 27.
void Bodega::atenderReserva( const std::string & nombre, const std::string & cantidadTexto ) {
    if ( nombre == "silenciar" ) {
        // Caso de prueba: esta bodega no responde, para que el
        // intermediario agote sus reintentos y envie MSG_ERR_COMM.
        rotular( idInt_, miId_, "(reserva de " + nombre + ")",
                  "(" + nombre_ + " no va a responder: simulando falla)" );
        return;
    }

    auto it = std::find_if( productos_.begin(), productos_.end(),
                             [ & ]( const Producto & p ) { return p.nombre == nombre; } );

    if ( it == productos_.end() ) {
        std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_PROD_NOT_FOUND, { nombre } );
        responder( msg, "no esta en " + nombre_ );
        return;
    }

    int pedido = std::stoi( cantidadTexto );
    int reservado = std::min( pedido, it->stock );
    it->stock -= reservado;

    std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_RESERVED,
                                     { it->nombre, formatearPrecio( it->precio ), std::to_string( reservado ) } );

    std::string explicacion;
    if ( reservado == pedido ) {
        explicacion = "reserva completa, quedan " + std::to_string( it->stock );
    } else if ( reservado > 0 ) {
        explicacion = "reserva parcial: solo habia " + std::to_string( reservado );
    } else {
        explicacion = "sin stock, se reservan 0";
    }
    responder( msg, explicacion );
}

void Bodega::ejecutar() {
    anunciarse();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( idInt_, miId_ ) );
        } catch ( const std::runtime_error & ) {
            return;
        }

        std::string linea( msg.texto );
        MensajeProtocolo mp;
        try {
            mp = parsearMensaje( linea );
        } catch ( const std::invalid_argument & ) {
            continue; // el intermediario ya valida lo que envia, asi que esto no deberia pasar
        }

        if ( mp.tipo == MSG_DEATH ) {
            rotular( idInt_, miId_, linea, "orden de cierre: " + nombre_ + " termina su hilo" );
            return;
        } else if ( mp.tipo == MSG_REQ_CATEGORIES ) {
            atenderCategorias();
        } else if ( mp.tipo == MSG_REQ_PRODUCTS && !mp.campos.empty() ) {
            atenderProductos( mp.campos[ 0 ] );
        } else if ( mp.tipo == MSG_RESERVE_STOCK && mp.campos.size() >= 2 ) {
            atenderReserva( mp.campos[ 0 ], mp.campos[ 1 ] );
        }
    }
}
