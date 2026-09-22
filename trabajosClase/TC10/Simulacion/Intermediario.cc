#include <sstream>
#include <iomanip>

#include "Intermediario.h"
#include "Bitacora.h"
#include "Validaciones.h"

Intermediario::Intermediario( Buzon & buzon ) : buzon_( buzon ) {
}

// Recibe los dos ANNOUNCE (uno por bodega) y arma el mapa categoria -> bodega.
void Intermediario::registrarBodegas() {
    for ( int i = 0; i < NUM_BODEGAS; ++i ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;
        buzon_.Recibir( &msg, sizeof( msg.texto ), CANAL_REGISTRO );
        std::string linea( msg.texto );

        MensajeProtocolo mp = parsearMensaje( linea );
        int idBodega = std::stoi( mp.campos[ 0 ] );
        std::string nombreBodega = mp.campos[ 1 ];
        std::string categoriasTexto = mp.campos[ 2 ];

        nombreBodega_[ idBodega ] = nombreBodega;

        std::istringstream cats( categoriasTexto );
        std::string categoria;
        while ( std::getline( cats, categoria, ',' ) ) {
            categoriaBodega_[ categoria ] = idBodega;
        }

        rotular( idBodega, ID_INTERMEDIARIO, linea, "bodega registrada" );
    }
}

void Intermediario::enviarErrFormat( const std::string & tipoOriginal, const std::string & campo,
                                       const std::string & valor ) {
    std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_ERR_FORMAT,
                                     { tipoOriginal, campo, valor } );
    buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "campo no cumple regex" );
}

void Intermediario::enviarErrComm( const std::string & tipoPendiente, int intentos ) {
    std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_ERR_COMM,
                                     { ID_TXT_BODEGA, tipoPendiente, std::to_string( intentos ) } );
    buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "la bodega no respondio" );
}

void Intermediario::manejarReCat( const std::string & categoria ) {
    if ( !validarCategoria( categoria ) ) {
        enviarErrFormat( "10", "categoria", categoria );
        return;
    }

    auto it = categoriaBodega_.find( categoria );
    if ( it == categoriaBodega_.end() ) {
        std::string texto = "No hay productos disponibles en la categoria \"" + categoria + "\"";
        std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_LIST_EMPT, { texto } );
        buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "ninguna bodega atiende esa categoria" );
        return;
    }

    int idBodega = it->second;
    std::string fwd = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_BODEGA, MSG_REQ_CAT_BOD, { categoria } );
    buzon_.Enviar( fwd.c_str(), canal( ID_INTERMEDIARIO, idBodega ) );
    rotular( ID_INTERMEDIARIO, idBodega, fwd, "reenviando a " + nombreBodega_[ idBodega ] );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( idBodega, ID_INTERMEDIARIO ) );
    MensajeProtocolo mpResp = parsearMensaje( std::string( resp.texto ) );

    if ( mpResp.campos[ 0 ] == "0" ) {
        std::string texto = "No hay productos disponibles en la categoria \"" + categoria + "\"";
        std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_LIST_EMPT, { texto } );
        buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "categoria vacia" );
    } else {
        std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_LIST, mpResp.campos );
        buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "listado reenviado al cliente" );
    }
}

void Intermediario::manejarReProd( const std::string & nombre ) {
    if ( !validarProducto( nombre ) ) {
        enviarErrFormat( "20", "nombre_producto", nombre );
        return;
    }

    int idsBodega[] = { ID_BODEGA1, ID_BODEGA2 };

    for ( int idBodega : idsBodega ) {
        std::string fwd = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_BODEGA, MSG_REQ_PROD_BOD, { nombre } );
        buzon_.Enviar( fwd.c_str(), canal( ID_INTERMEDIARIO, idBodega ) );
        rotular( ID_INTERMEDIARIO, idBodega, fwd, "consultando a " + nombreBodega_[ idBodega ] );

        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
        int intentos = 2;
        int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ),
                                           canal( idBodega, ID_INTERMEDIARIO ), intentos, 300 );

        if ( st == -1 ) {
            enviarErrComm( "24", intentos );
            return;
        }

        MensajeProtocolo mpResp = parsearMensaje( std::string( resp.texto ) );

        if ( mpResp.tipo == MSG_PROD_FOUND ) {
            std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_DETAIL, mpResp.campos );
            buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
            rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "detalle de producto" );
            return;
        }
        // MSG_PROD_NOT_FOUND: seguimos con la siguiente bodega
    }

    std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_NOT_AVAIL,
                                     { nombre + " no encontrado" } );
    buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "producto no disponible en ninguna bodega" );
}

void Intermediario::manejarAddCart( const std::string & producto, const std::string & countTexto ) {
    if ( !validarProducto( producto ) ) {
        enviarErrFormat( "30", "producto", producto );
        return;
    }
    if ( !validarCount( countTexto ) ) {
        enviarErrFormat( "30", "count", countTexto );
        return;
    }

    int count = std::stoi( countTexto );
    int idsBodega[] = { ID_BODEGA1, ID_BODEGA2 };

    for ( int idBodega : idsBodega ) {
        std::string fwd = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_BODEGA, MSG_REQ_PROD_BOD, { producto } );
        buzon_.Enviar( fwd.c_str(), canal( ID_INTERMEDIARIO, idBodega ) );
        rotular( ID_INTERMEDIARIO, idBodega, fwd, "consultando a " + nombreBodega_[ idBodega ] + " para el carrito" );

        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
        int intentos = 2;
        int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ),
                                           canal( idBodega, ID_INTERMEDIARIO ), intentos, 300 );

        if ( st == -1 ) {
            enviarErrComm( "24", intentos );
            return;
        }

        MensajeProtocolo mpResp = parsearMensaje( std::string( resp.texto ) );

        if ( mpResp.tipo == MSG_PROD_FOUND ) {
            const std::string & nom = mpResp.campos[ 0 ];
            const std::string & precioTexto = mpResp.campos[ 1 ];
            int stock = std::stoi( mpResp.campos[ 2 ] );

            if ( stock < count ) {
                std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_ERR_STOCK,
                                                 { producto, mpResp.campos[ 2 ], countTexto } );
                buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
                rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "no hay stock suficiente" );
                return;
            }

            carrito_.push_back( { nom, std::stof( precioTexto ), count } );

            std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_ADDED,
                                             { nom, precioTexto, countTexto } );
            buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
            rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "producto agregado al carrito" );
            return;
        }
        // MSG_PROD_NOT_FOUND: seguimos con la siguiente bodega
    }

    std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_PROD_NOT_AVAIL,
                                     { producto + " no encontrado" } );
    buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "producto no disponible para agregar" );
}

void Intermediario::manejarRequestFactura( const std::string & countTexto ) {
    if ( countTexto == "0" || carrito_.empty() ) {
        std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_FACT_VACIA,
                                         { "El carrito esta vacio" } );
        buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "carrito vacio" );
        return;
    }

    float total = 0.0f;
    std::ostringstream items;
    for ( const auto & item : carrito_ ) {
        float subtotal = item.precio * item.cantidad;
        total += subtotal;
        items << item.nombre << "," << item.cantidad << ","
              << std::fixed << std::setprecision( 2 ) << subtotal << ";";
    }

    std::ostringstream totalStr;
    totalStr << std::fixed << std::setprecision( 2 ) << total;

    std::string msg = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_CLIENTE, MSG_FACTURA,
                                     { totalStr.str(), std::to_string( carrito_.size() ), items.str() } );
    buzon_.Enviar( msg.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, msg, "factura proforma generada" );

    carrito_.clear();
}

void Intermediario::ejecutar() {
    registrarBodegas();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
        } catch ( const std::runtime_error & ) {
            return;
        }

        std::string linea( msg.texto );
        rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "peticion recibida" );

        MensajeProtocolo mp = parsearMensaje( linea );

        if ( mp.tipo == MSG_DEATH ) {
            rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "orden de cierre recibida del cliente" );
            std::string death = armarMensaje( ID_TXT_INTERMEDIARIO, ID_TXT_BODEGA, MSG_DEATH, {} );
            buzon_.Enviar( death.c_str(), canal( ID_INTERMEDIARIO, ID_BODEGA1 ) );
            buzon_.Enviar( death.c_str(), canal( ID_INTERMEDIARIO, ID_BODEGA2 ) );
            return;
        } else if ( mp.tipo == MSG_REQ_CAT_CLI && !mp.campos.empty() ) {
            manejarReCat( mp.campos[ 0 ] );
        } else if ( mp.tipo == MSG_REQ_PROD_CLI && !mp.campos.empty() ) {
            manejarReProd( mp.campos[ 0 ] );
        } else if ( mp.tipo == MSG_ADD_CART && mp.campos.size() >= 2 ) {
            manejarAddCart( mp.campos[ 0 ], mp.campos[ 1 ] );
        } else if ( mp.tipo == MSG_REQUEST_FACTURA && !mp.campos.empty() ) {
            manejarRequestFactura( mp.campos[ 0 ] );
        }
    }
}