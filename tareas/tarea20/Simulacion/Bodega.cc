#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <mutex>

#include "Bodega.h"
#include "Bitacora.h"

// en el archivo el precio se guarda en centavos
static std::string formatearPrecio( uint32_t centavos ) {
    std::ostringstream out;
    out << std::fixed << std::setprecision( 2 ) << centavos / 100.0;
    return out.str();
}

// busca la bodega en el directorio del filesystem y devuelve su i-nodo
static uint16_t inodoDe( Filesystem & fs, const std::string & nombre, const std::string & ruta ) {
    EntradaDirectorio entrada;
    if ( !fs.buscarBodega( nombre, entrada ) ) {
        throw std::runtime_error( nombre + " no esta en el directorio de " + ruta );
    }
    return entrada.inodo;
}

Bodega::Bodega( Buzon & buzon, int miId, const std::string & nombre,
                 int idIntermediario, const std::string & idTextoIntermediario,
                 Filesystem & fs, const std::string & rutaArchivo )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ),
      idInt_( idIntermediario ), txtInt_( idTextoIntermediario ),
      fs_( fs ), rutaArchivo_( rutaArchivo ),
      inodo_( inodoDe( fs, nombre, rutaArchivo ) ),
      archivo_( fs, inodo_, nombre ) {
}

void Bodega::rotularAcceso( const std::string & accion ) {
    Inodo inodo;
    {
        // leerInodo no toma el mutex, y las otras bodegas de la isla usan el mismo archivo
        std::lock_guard<std::mutex> lock( fs_.mutex() );
        fs_.leerInodo( inodo_, inodo );
    }
    std::string bloques;
    for ( int i = 0; i < inodo.cantidadBloques; i++ ) {
        bloques += ( i == 0 ? "" : "," ) + std::to_string( inodo.bloques[ i ] );
    }
    rotularDisco( miId_, rutaArchivo_, accion + " (i-nodo " + std::to_string( inodo_ ) + ", " +
                  std::to_string( inodo.tamano ) + " bytes, bloques " + bloques + ")" );
}

// las categorias salen de las lineas del archivo: una categoria existe si alguna linea la menciona
std::string Bodega::listaCategorias() {
    std::vector<std::string> categorias = archivo_.listarCategorias();
    rotularAcceso( "lee las categorias de " + nombre_ );

    std::string lista;
    for ( const auto & categoria : categorias ) {
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
    std::vector<LineaProducto> productos = archivo_.listarProductos( categoria );
    rotularAcceso( "lee " + std::to_string( productos.size() ) + " lineas de " + categoria );

    std::ostringstream lista;
    int cantidad = 0;
    for ( const auto & p : productos ) {
        if ( cantidad > 0 ) lista << ";";
        lista << p.nombre << "," << formatearPrecio( p.precioCentavos ) << "," << p.cantidad;
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

    // reservarStock lee el archivo, resta y lo vuelve a escribir, todo con el mutex del filesystem
    int pedido = std::stoi( cantidadTexto );
    int reservado = 0;
    uint32_t precioCentavos = 0;
    if ( !archivo_.reservarStock( nombre, pedido, reservado, precioCentavos ) ) {
        rotularAcceso( "busca " + nombre + " y no lo encuentra" );
        std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_PROD_NOT_FOUND, { nombre } );
        responder( msg, "no esta en " + nombre_ );
        return;
    }

    // se vuelve a leer del archivo para mostrar el stock que quedo guardado
    int quedan = 0;
    for ( const auto & p : archivo_.leerLineas() ) {
        if ( p.nombre == nombre ) quedan = p.cantidad;
    }
    rotularAcceso( reservado > 0 ? "descuenta " + std::to_string( reservado ) + " de " + nombre +
                                   " y reescribe la bodega, quedan " + std::to_string( quedan )
                                 : "lee " + nombre + ": no hay stock, no se escribe nada" );

    std::string msg = armarMensaje( ID_TXT_BODEGA, txtInt_, MSG_RESERVED,
                                     { nombre, formatearPrecio( precioCentavos ), std::to_string( reservado ) } );

    std::string explicacion;
    if ( reservado == pedido ) {
        explicacion = "reserva completa, quedan " + std::to_string( quedan );
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
