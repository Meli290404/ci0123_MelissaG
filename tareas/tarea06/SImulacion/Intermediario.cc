#include <sstream>
#include <iomanip>

#include "Intermediario.h"
#include "Bitacora.h"
#include "Validaciones.h"

Intermediario::Intermediario( Buzon & buzon ) : buzon_( buzon ), idBodega_( -1 ) {
}

// Recibe el HOLA de la bodega y aprende su id, nombre y categorías.
// Formato esperado: HOLA <id> <nombre> <cat1,cat2,...>
void Intermediario::registrarBodega() {
    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;
    buzon_.Recibir( &msg, sizeof( msg.texto ), CANAL_REGISTRO );
    std::string linea( msg.texto );

    std::istringstream iss( linea );
    std::string verbo, idTexto, categoriasTexto;
    iss >> verbo >> idTexto >> nombreBodega_ >> categoriasTexto;

    idBodega_ = std::stoi( idTexto );

    // Partir "cat1,cat2,cat3" por comas
    std::istringstream cats( categoriasTexto );
    std::string categoria;
    while ( std::getline( cats, categoria, ',' ) ) {
        categoriasAtendidas_.insert( categoria );
    }

    rotular( idBodega_, ID_INTERMEDIARIO, linea, "bodega registrada" );
}

void Intermediario::manejarReCat( const std::string & categoria ) {
    if ( !validarCategoria( categoria ) ) {
        std::ostringstream err;
        err << "ERR_FORMAT RE_CAT categoria " << categoria;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), "categoria no cumple regex" );
        return;
    }

    if ( categoriasAtendidas_.find( categoria ) == categoriasAtendidas_.end() ) {
        // Caso en donde la categoria no está en la bodega
        std::ostringstream vacio;
        vacio << "PROD_LIST_EMPT No hay productos disponibles en la categoria \"" << categoria << "\"";
        buzon_.Enviar( vacio.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, vacio.str(), "la bodega no atiende esa categoria" );
        return;
    }

    std::ostringstream fwd;
    fwd << "LIST_PROD " << categoria;
    buzon_.Enviar( fwd.str().c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, fwd.str(), "reenviando a " + nombreBodega_ );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( idBodega_, ID_INTERMEDIARIO ) );
    std::string respuesta( resp.texto );

    if ( respuesta == "PROD_LIST 0" ) {
        std::ostringstream vacio;
        vacio << "PROD_LIST_EMPT No hay productos disponibles en la categoria \"" << categoria << "\"";
        buzon_.Enviar( vacio.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, vacio.str(), "categoria vacia" );
    } else {
        buzon_.Enviar( respuesta.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, respuesta, "listado reenviado al cliente" );
    }
}

void Intermediario::manejarReProd( const std::string & nombre ) {
    if ( !validarProducto( nombre ) ) {
        std::ostringstream err;
        err << "ERR_FORMAT RE_PROD nombre_producto " << nombre;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), "nombre de producto no cumple regex" );
        return;
    }

    std::ostringstream fwd;
    fwd << "FIND_PROD " << nombre;
    buzon_.Enviar( fwd.str().c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, fwd.str(), "consultando a " + nombreBodega_ );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    int intentos = 2;
    int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ),
                                       canal( idBodega_, ID_INTERMEDIARIO ), intentos, 300 );

    if ( st == -1 ) {
        std::ostringstream err;
        err << "ERR_COMM " << nombreBodega_ << " FIND_PROD " << intentos;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), nombreBodega_ + " no respondio" );
        return;
    }

    std::string respuesta( resp.texto );
    std::istringstream issResp( respuesta );
    std::string verboResp;
    issResp >> verboResp;

    if ( verboResp == "PROD_FOUND" ) {
        std::string nom, precio, stock, descripcion;
        issResp >> nom >> precio >> stock;
        std::getline( issResp, descripcion );
        std::ostringstream out;
        out << "PROD " << nom << " " << precio << " " << stock << " " << descripcion;
        buzon_.Enviar( out.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, out.str(), "detalle de producto" );
    } else { // PROD_NOT_FOUND
        std::ostringstream out;
        out << "PROD_NOT_AVAIL " << nombre << " no encontrado";
        buzon_.Enviar( out.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, out.str(), "producto no disponible" );
    }
}

void Intermediario::manejarAddCart( const std::string & producto, const std::string & countTexto ) {
    if ( !validarProducto( producto ) ) {
        std::ostringstream err;
        err << "ERR_FORMAT ADD_CART producto " << producto;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), "nombre de producto no cumple regex" );
        return;
    }

    if ( !validarCount( countTexto ) ) {
        std::ostringstream err;
        err << "ERR_FORMAT ADD_CART count " << countTexto;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), "count no cumple regex" );
        return;
    }

    int count = std::stoi( countTexto );

    std::ostringstream fwd;
    fwd << "FIND_PROD " << producto;
    buzon_.Enviar( fwd.str().c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, fwd.str(), "consultando a " + nombreBodega_ + " para el carrito" );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    int intentos = 2;
    int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ),
                                       canal( idBodega_, ID_INTERMEDIARIO ), intentos, 300 );

    if ( st == -1 ) {
        std::ostringstream err;
        err << "ERR_COMM " << nombreBodega_ << " FIND_PROD " << intentos;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), nombreBodega_ + " no respondio" );
        return;
    }

    std::string respuesta( resp.texto );
    std::istringstream issResp( respuesta );
    std::string verboResp;
    issResp >> verboResp;

    if ( verboResp != "PROD_FOUND" ) {
        std::ostringstream out;
        out << "PROD_NOT_AVAIL " << producto << " no encontrado";
        buzon_.Enviar( out.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, out.str(), "producto no disponible para agregar" );
        return;
    }

    std::string nom, precioTexto, stockTexto;
    issResp >> nom >> precioTexto >> stockTexto;
    int stock = std::stoi( stockTexto );

    if ( stock < count ) {
        std::ostringstream err;
        err << "ERR_STOCK " << producto << " " << stock << " " << count;
        buzon_.Enviar( err.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err.str(), "no hay stock suficiente" );
        return;
    }

    carrito_.push_back( { nom, std::stof( precioTexto ), count } );

    std::ostringstream out;
    out << "PROD_ADDED " << nom << " " << precioTexto << " " << count;
    buzon_.Enviar( out.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, out.str(), "producto agregado al carrito" );
}

void Intermediario::manejarGetFact( const std::string & countTexto ) {
    // La factura se arma con lo que ya se confirmo en los ADD_CART anteriores,
    if ( countTexto == "0" || carrito_.empty() ) {
        buzon_.Enviar( "FACT_VACIA El carrito esta vacio", canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, "FACT_VACIA", "carrito vacio" );
        return;
    }

    float total = 0.0f;
    std::ostringstream lista;
    for ( const auto & item : carrito_ ) {
        float subtotal = item.precio * item.cantidad;
        total += subtotal;
        lista << item.nombre << "," << item.cantidad << ","
              << std::fixed << std::setprecision( 2 ) << subtotal << ";";
    }

    std::ostringstream out;
    out << "FACTURA " << std::fixed << std::setprecision( 2 ) << total
        << " " << carrito_.size() << " " << lista.str();
    buzon_.Enviar( out.str().c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, out.str(), "factura proforma generada" );

    carrito_.clear(); // el pedido quedo cerrado
}

void Intermediario::ejecutar() {
    registrarBodega();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
        } catch ( const std::runtime_error & ) {
            return; // el buzon fue destruido: salida de respaldo
        }

        std::string linea( msg.texto );
        rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "peticion recibida" );

        std::istringstream iss( linea );
        std::string verbo;
        iss >> verbo;

        if ( verbo == "EXIT" ) {
            rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "orden de cierre recibida del cliente" );
            buzon_.Enviar( "EXIT", canal( ID_INTERMEDIARIO, idBodega_ ) );
            return;
        } else if ( verbo == "RE_CAT" ) {
            std::string categoria;
            iss >> categoria;
            manejarReCat( categoria );
        } else if ( verbo == "RE_PROD" ) {
            std::string nombre;
            iss >> nombre;
            manejarReProd( nombre );
        } else if ( verbo == "ADD_CART" ) {
            std::string producto, countTexto;
            iss >> producto >> countTexto;
            manejarAddCart( producto, countTexto );
        } else if ( verbo == "GET_FACT" ) {
            std::string countTexto;
            iss >> countTexto;
            manejarGetFact( countTexto );
        }
    }
}