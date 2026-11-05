#include <cstdio>
#include <cstring>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "Intermediario.h"
#include "Bitacora.h"
#include "Validaciones.h"

static const int REINTENTOS_BODEGA = 2;     // intentos extra despues del primero
static const int ESPERA_BODEGA_MS = 300;    // espera entre intentos de RecibirConEspera
static const int INTENTOS_ISLA = 3;         // el mancomunado recomienda hasta 3 intentos
static const int SONDEOS_ISLA = 50;         // 50 sondeos de 100 ms: los 5 s de espera que recomienda el protocolo
static const int ESPERA_ISLA_MS = 100;

static std::string formatearMonto( float monto ) {
    std::ostringstream out;
    out << std::fixed << std::setprecision( 2 ) << monto;
    return out.str();
}

static std::string minusculas( std::string texto ) {
    for ( char & c : texto ) c = (char) tolower( (unsigned char) c );
    return texto;
}

static std::vector<std::string> separar( const std::string & texto, char separador ) {
    std::vector<std::string> partes;
    std::istringstream iss( texto );
    std::string parte;
    while ( std::getline( iss, parte, separador ) ) {
        if ( !parte.empty() ) partes.push_back( parte );
    }
    return partes;
}

static DatosRed armarDatos( long canalRespuesta, int idOrigen, const std::string & texto ) {
    DatosRed datos;
    datos.canalRespuesta = canalRespuesta;
    datos.idOrigen = idOrigen;
    strncpy( datos.texto, texto.c_str(), TAM_MAX_MENSAJE - 1 );
    datos.texto[ TAM_MAX_MENSAJE - 1 ] = '\0';
    return datos;
}

Intermediario::Intermediario( Buzon & buzon, int miId, const std::string & idTexto, const std::string & ip,
                               int cantidadBodegas, std::vector<int> idsIslas )
    : buzon_( buzon ), miId_( miId ), idTexto_( idTexto ), ip_( ip ),
      cantidadBodegas_( cantidadBodegas ), idsIslas_( std::move( idsIslas ) ) {
}

// ---------------------------------------------------------------------------
// arranque y cierre
// ---------------------------------------------------------------------------

void Intermediario::iniciar() {
    registrarBodegas();
    escucha_ = std::thread( &Intermediario::escuchar, this );
}

// Formato: 60/ID_MSG/id_isla/ip/puerto. ID_MSG es unico por mensaje de esta isla.
void Intermediario::anunciarse() {
    char idMensaje[ 8 ];
    snprintf( idMensaje, sizeof( idMensaje ), "%04d", siguienteIdMensaje_++ );

    std::string msg = armarMancomunado( MAN_ANNOUNCE, { idMensaje, idTexto_, ip_, std::to_string( PUERTO_MANCOMUNADO ) } );
    rotular( miId_, ID_BROADCAST, msg, "(mancomunado) anuncio por broadcast dirigido, udp 9090" );
    enviarBroadcast( msg );
}

void Intermediario::conocerIslas() {
    std::map<int, IslaRemota> conocidas;
    {
        // cada isla espera hasta haber recibido el anuncio de todas las demas
        std::unique_lock<std::mutex> lock( mtxIslas_ );
        cvIslas_.wait( lock, [ & ] { return islas_.size() + 1 >= idsIslas_.size(); } );
        conocidas = islas_;
    }

    for ( const auto & par : conocidas ) {
        MensajeMancomunado resp;
        if ( !solicitarIsla( par.first, armarMancomunado( MAN_GET_CATEGORIES, {} ),
                             "categorias de " + par.second.idIsla, resp ) ) {
            continue;
        }
        if ( resp.tipo != MAN_ACK || resp.campos.empty() ) continue;
        for ( const auto & categoria : separar( resp.campos[ 0 ], ',' ) ) {
            islaDeCategoria_[ categoria ] = par.first;
        }
    }
}

void Intermediario::despedirse() {
    std::string msg = armarMancomunado( MAN_ANNOUNCE_BYE, { idTexto_ } );
    rotular( miId_, ID_BROADCAST, msg, "(mancomunado) despedida por broadcast dirigido, udp 9090" );
    enviarBroadcast( msg );

    // la escucha termina cuando le llega el eco de esta misma despedida
    if ( escucha_.joinable() ) escucha_.join();
}

void Intermediario::terminar() {
    std::string death = armarMensaje( idTexto_, ID_TXT_BODEGA, MSG_DEATH, {} );
    for ( int idBodega : idsBodega_ ) {
        buzon_.Enviar( death.c_str(), canal( miId_, idBodega ) );
    }
}

// ---------------------------------------------------------------------------
// protocolo interno con las bodegas propias
// ---------------------------------------------------------------------------

// Recibe un anuncio por bodega y arma el mapa categoria -> bodega.
void Intermediario::registrarBodegas() {
    for ( int i = 0; i < cantidadBodegas_; ++i ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;
        buzon_.Recibir( &msg, sizeof( msg.texto ), canalRegistro( miId_ ) );
        std::string linea( msg.texto );

        MensajeProtocolo mp = parsearMensaje( linea );
        int idBodega = std::stoi( mp.campos[ 0 ] );
        nombreBodega_[ idBodega ] = mp.campos[ 1 ];
        idsBodega_.push_back( idBodega );

        for ( const auto & categoria : separar( mp.campos[ 2 ], ',' ) ) {
            categoriaBodega_[ categoria ] = idBodega;
        }

        rotular( idBodega, miId_, linea, "bodega registrada" );
    }

    // los anuncios llegan en el orden en que arrancan los hilos; se ordenan
    // por id para que las bodegas se consulten siempre en el mismo orden
    std::sort( idsBodega_.begin(), idsBodega_.end() );
}

bool Intermediario::consultarBodega( int idBodega, int tipo, const std::vector<std::string> & campos,
                                      const std::string & explicacion, MensajeProtocolo & respuesta ) {
    // la escucha (otras islas) y la atencion del cliente no pueden mezclar respuestas de la misma bodega
    std::lock_guard<std::mutex> lock( mtxBodegas_ );

    std::string msg = armarMensaje( idTexto_, ID_TXT_BODEGA, tipo, campos );
    rotular( miId_, idBodega, msg, explicacion );
    buzon_.Enviar( msg.c_str(), canal( miId_, idBodega ) );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ), canal( idBodega, miId_ ),
                                      REINTENTOS_BODEGA + 1, ESPERA_BODEGA_MS );
    if ( st == -1 ) {
        rotular( idBodega, miId_, "(sin respuesta)", nombreBodega_[ idBodega ] + " no respondio" );
        return false;
    }

    respuesta = parsearMensaje( std::string( resp.texto ) );
    return true;
}

// Recorre las categorias propias con el mensaje 20 hasta encontrar el producto.
// El nombre se compara en minusculas, como pide el protocolo mancomunado.
bool Intermediario::buscarProductoLocal( const std::string & nombreMinusculas, int & idBodega, std::string & nombre,
                                          float & precio, int & stock ) {
    for ( const auto & par : categoriaBodega_ ) {
        MensajeProtocolo resp;
        if ( !consultarBodega( par.second, MSG_REQ_PRODUCTS, { par.first },
                               "buscando " + nombreMinusculas + " en " + par.first, resp ) ) {
            continue;
        }
        if ( resp.tipo != MSG_PRODUCT_LIST || resp.campos.size() < 2 ) continue;

        for ( const auto & item : separar( resp.campos[ 1 ], ';' ) ) {
            std::vector<std::string> partes = separar( item, ',' );
            if ( partes.size() == 3 && minusculas( partes[ 0 ] ) == nombreMinusculas ) {
                idBodega = par.second;
                nombre = partes[ 0 ];
                precio = std::stof( partes[ 1 ] );
                stock = std::stoi( partes[ 2 ] );
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// protocolo interno con el cliente
// ---------------------------------------------------------------------------

void Intermediario::atenderCliente() {
    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_CLIENTE, miId_ ) );
        } catch ( const std::runtime_error & ) {
            return;
        }

        std::string linea( msg.texto );
        rotular( ID_CLIENTE, miId_, linea, "peticion recibida" );

        MensajeProtocolo mp;
        try {
            mp = parsearMensaje( linea );
        } catch ( const std::invalid_argument & ) {
            responderCliente( MSG_ERR_FORMAT, { "00", "mensaje", "invalido" }, "mensaje mal formado" );
            continue;
        }

        if ( mp.tipo == MSG_DEATH ) {
            rotular( ID_CLIENTE, miId_, linea, "orden de cierre recibida del cliente" );
            return;
        } else if ( mp.tipo == MSG_REQ_CATEGORIES ) {
            manejarCategorias();
        } else if ( mp.tipo == MSG_REQ_PRODUCTS && !mp.campos.empty() ) {
            manejarProductos( mp.campos[ 0 ] );
        } else if ( mp.tipo == MSG_ADD_CART && mp.campos.size() >= 2 ) {
            manejarAgregar( mp.campos[ 0 ], mp.campos[ 1 ] );
        } else if ( mp.tipo == MSG_REQUEST_FACTURA ) {
            manejarFactura( mp.campos );
        } else {
            responderCliente( MSG_ERR_FORMAT, { std::to_string( mp.tipo ), "tipo_mensaje", std::to_string( mp.tipo ) },
                              "tipo de mensaje no soportado" );
        }
    }
}

void Intermediario::responderCliente( int tipo, const std::vector<std::string> & campos,
                                       const std::string & explicacion ) {
    std::string msg = armarMensaje( idTexto_, ID_TXT_CLIENTE, tipo, campos );
    rotular( miId_, ID_CLIENTE, msg, explicacion );
    buzon_.Enviar( msg.c_str(), canal( miId_, ID_CLIENTE ) );
}

bool Intermediario::campoValido( const std::string & tipoOriginal, const std::string & campo,
                                  const std::string & valor, size_t tamMax,
                                  bool ( *validador )( const std::string & ) ) {
    if ( valor.size() > tamMax ) {
        responderCliente( MSG_ERR_SIZE, { tipoOriginal, campo, std::to_string( valor.size() ), std::to_string( tamMax ) },
                          "campo " + campo + " excede el tamano maximo" );
        return false;
    }
    if ( !validador( valor ) ) {
        responderCliente( MSG_ERR_FORMAT, { tipoOriginal, campo, valor }, "campo " + campo + " no cumple regex" );
        return false;
    }
    return true;
}

// Mensaje 10: categorias de las bodegas propias con el protocolo interno y de
// las otras islas con MAN_GET_CATEGORIES; al cliente le llega todo en un solo 11.
void Intermediario::manejarCategorias() {
    std::string lista;
    for ( int idBodega : idsBodega_ ) {
        MensajeProtocolo resp;
        if ( !consultarBodega( idBodega, MSG_REQ_CATEGORIES, {}, "pidiendo categorias a " + nombreBodega_[ idBodega ], resp ) ) {
            responderCliente( MSG_ERR_COMM, { ID_TXT_BODEGA, std::to_string( MSG_REQ_CATEGORIES ),
                              std::to_string( REINTENTOS_BODEGA ) }, nombreBodega_[ idBodega ] + " no respondio" );
            return;
        }
        if ( resp.tipo != MSG_CATEGORY_LIST || resp.campos.empty() ) continue;
        if ( !lista.empty() ) lista += ",";
        lista += resp.campos[ 0 ];
    }

    std::map<int, IslaRemota> conocidas;
    {
        std::lock_guard<std::mutex> lock( mtxIslas_ );
        conocidas = islas_;
    }

    for ( const auto & par : conocidas ) {
        MensajeMancomunado resp;
        if ( !solicitarIsla( par.first, armarMancomunado( MAN_GET_CATEGORIES, {} ),
                             "categorias de " + par.second.idIsla, resp ) ) {
            responderCliente( MSG_ERR_COMM, { par.second.idIsla, std::to_string( MAN_GET_CATEGORIES ),
                              std::to_string( INTENTOS_ISLA - 1 ) }, par.second.idIsla + " no respondio" );
            return;
        }
        if ( resp.tipo != MAN_ACK || resp.campos.empty() ) continue;

        for ( const auto & categoria : separar( resp.campos[ 0 ], ',' ) ) {
            // el mancomunado permite digitos y guion bajo; el protocolo interno solo letras
            if ( !validarCategoria( categoria ) ) continue;
            islaDeCategoria_[ categoria ] = par.first;
            if ( !lista.empty() ) lista += ",";
            lista += categoria;
        }
    }
    responderCliente( MSG_CATEGORY_LIST, { lista }, "categorias de la isla 4 y de las otras islas" );
}

// Mensaje 20: si la categoria es de una bodega propia se usa el protocolo interno;
// si es de otra isla se pide con MAN_GET_PRODUCTS. Al cliente le llega 22, o 23 si no hay productos.
void Intermediario::manejarProductos( const std::string & categoria ) {
    if ( !campoValido( "20", "categoria", categoria, 20, validarCategoria ) ) return;

    std::string sinProductos = "No hay productos disponibles en la categoria \"" + categoria + "\"";

    auto local = categoriaBodega_.find( categoria );
    if ( local != categoriaBodega_.end() ) {
        MensajeProtocolo resp;
        if ( !consultarBodega( local->second, MSG_REQ_PRODUCTS, { categoria },
                               "pidiendo productos a " + nombreBodega_[ local->second ], resp ) ) {
            responderCliente( MSG_ERR_COMM, { ID_TXT_BODEGA, std::to_string( MSG_REQ_PRODUCTS ),
                              std::to_string( REINTENTOS_BODEGA ) }, nombreBodega_[ local->second ] + " no respondio" );
            return;
        }
        if ( resp.tipo != MSG_PRODUCT_LIST || resp.campos.empty() || resp.campos[ 0 ] == "0" ) {
            responderCliente( MSG_NO_PRODUCTS, { sinProductos }, "la bodega respondio 22 con count 0" );
            return;
        }
        responderCliente( MSG_PRODUCT_LIST, resp.campos, "listado reenviado al cliente" );
        return;
    }

    auto remota = islaDeCategoria_.find( categoria );
    if ( remota == islaDeCategoria_.end() ) {
        responderCliente( MSG_NO_PRODUCTS, { sinProductos }, "ninguna bodega ni isla atiende esa categoria" );
        return;
    }

    int idIsla = remota->second;
    MensajeMancomunado resp;
    if ( !solicitarIsla( idIsla, armarMancomunado( MAN_GET_PRODUCTS, { categoria } ),
                         "productos de " + categoria + " en " + textoIsla( idIsla ), resp ) ) {
        responderCliente( MSG_ERR_COMM, { textoIsla( idIsla ), std::to_string( MAN_GET_PRODUCTS ),
                          std::to_string( INTENTOS_ISLA - 1 ) }, textoIsla( idIsla ) + " no respondio" );
        return;
    }

    // 01/0 o 03 de la otra isla se traducen al 23 del protocolo interno
    if ( resp.tipo != MAN_ACK || resp.campos.size() < 2 || resp.campos[ 0 ] == "0" ) {
        responderCliente( MSG_NO_PRODUCTS, { sinProductos }, textoIsla( idIsla ) + " no tiene productos en esa categoria" );
        return;
    }

    recordarProductos( idIsla, resp.campos[ 1 ] );
    responderCliente( MSG_PRODUCT_LIST, { resp.campos[ 0 ], resp.campos[ 1 ] },
                      "listado de " + textoIsla( idIsla ) + " traducido al protocolo interno" );
}

// Mensaje 30: primero se pide la reserva (50) a las bodegas propias; si ninguna
// tiene el producto se reserva en otra isla con MAN_RESERVE.
void Intermediario::manejarAgregar( const std::string & producto, const std::string & cantidadTexto ) {
    if ( !campoValido( "30", "producto", producto, 50, validarProducto ) ) return;
    if ( !campoValido( "30", "cantidad", cantidadTexto, 3, validarCount ) ) return;

    int pedido = std::stoi( cantidadTexto );

    for ( int idBodega : idsBodega_ ) {
        MensajeProtocolo resp;
        if ( !consultarBodega( idBodega, MSG_RESERVE_STOCK, { producto, cantidadTexto },
                               "reservando en " + nombreBodega_[ idBodega ], resp ) ) {
            responderCliente( MSG_ERR_COMM, { ID_TXT_BODEGA, std::to_string( MSG_RESERVE_STOCK ),
                              std::to_string( REINTENTOS_BODEGA ) }, nombreBodega_[ idBodega ] + " no respondio" );
            return;
        }

        if ( resp.tipo == MSG_PROD_NOT_FOUND ) continue;   // se pregunta a la siguiente bodega
        if ( resp.tipo != MSG_RESERVED || resp.campos.size() < 3 ) continue;

        int reservado = std::stoi( resp.campos[ 2 ] );
        float precio = std::stof( resp.campos[ 1 ] );

        if ( reservado > 0 ) {
            // si el producto ya estaba con el mismo precio se suman las cantidades
            bool encontrado = false;
            for ( auto & item : carrito_ ) {
                if ( item.nombre == resp.campos[ 0 ] && item.precio == precio && item.idIsla == 0 ) {
                    item.cantidad += reservado;
                    encontrado = true;
                }
            }
            if ( !encontrado ) carrito_.push_back( { resp.campos[ 0 ], precio, reservado, 0 } );
        }

        std::string explicacion;
        if ( reservado == pedido ) {
            explicacion = "producto agregado al carrito";
        } else if ( reservado > 0 ) {
            explicacion = "reserva parcial agregada al carrito";
        } else {
            explicacion = "sin stock: no se agrega al carrito";
        }
        responderCliente( MSG_CART_ADDED, { resp.campos[ 0 ], resp.campos[ 1 ], resp.campos[ 2 ] }, explicacion );
        return;
    }

    agregarRemoto( producto, cantidadTexto );
}

void Intermediario::agregarRemoto( const std::string & producto, const std::string & cantidadTexto ) {
    // el precio y la isla salen de un listado anterior; si no se conocen se busca en las otras islas
    if ( productoRemoto_.find( producto ) == productoRemoto_.end() && !buscarProductoRemoto( producto ) ) {
        responderCliente( MSG_PROD_NOT_AVAIL, { producto + " no encontrado" }, "ninguna bodega ni isla tiene el producto" );
        return;
    }

    const ProductoRemoto & remoto = productoRemoto_[ producto ];
    std::string isla = textoIsla( remoto.idIsla );
    std::string precioTexto = formatearMonto( remoto.precio );

    MensajeMancomunado resp;
    if ( !solicitarIsla( remoto.idIsla, armarMancomunado( MAN_RESERVE, { producto, cantidadTexto } ),
                         "reservando en " + isla, resp ) ) {
        responderCliente( MSG_ERR_COMM, { isla, std::to_string( MAN_RESERVE ), std::to_string( INTENTOS_ISLA - 1 ) },
                          isla + " no respondio" );
        return;
    }

    if ( resp.tipo == MAN_ACK ) {
        carrito_.push_back( { producto, remoto.precio, std::stoi( cantidadTexto ), remoto.idIsla } );
        responderCliente( MSG_CART_ADDED, { producto, precioTexto, cantidadTexto },
                          "reservado en " + isla + ", agregado al carrito" );
    } else if ( resp.tipo == MAN_ERROR && !resp.campos.empty() && resp.campos[ 0 ] == std::to_string( ERR_STOCK ) ) {
        // el mancomunado no hace reservas parciales: 02/103 equivale a reservar 0
        responderCliente( MSG_CART_ADDED, { producto, precioTexto, "0" }, "stock insuficiente en " + isla + ": no se agrega" );
    } else {
        responderCliente( MSG_PROD_NOT_AVAIL, { producto + " no encontrado" }, isla + " rechazo la reserva" );
    }
}

// Mensaje 40: los productos propios se facturan con lo ya reservado en carrito_; los de otra
// isla se compran con MAN_BUY, una compra por isla, y su detalle se agrega a la misma factura.
void Intermediario::manejarFactura( const std::vector<std::string> & campos ) {
    std::string countTexto = campos.empty() ? "0" : campos[ 0 ];
    if ( !campoValido( "40", "count", countTexto, 3, validarCount ) ) return;

    if ( countTexto == "0" || carrito_.empty() ) {
        responderCliente( MSG_FACT_VACIA, { "El carrito esta vacio" }, "carrito vacio" );
        return;
    }

    float total = 0.0f;
    int cantidadItems = 0;
    std::string detalle;
    std::map<int, std::vector<ItemCarrito>> porIsla;

    for ( const auto & item : carrito_ ) {
        if ( item.idIsla != 0 ) {
            porIsla[ item.idIsla ].push_back( item );
            continue;
        }
        float subtotal = item.precio * item.cantidad;
        total += subtotal;
        ++cantidadItems;
        if ( !detalle.empty() ) detalle += ";";
        detalle += item.nombre + "," + std::to_string( item.cantidad ) + "," + formatearMonto( subtotal );
    }

    for ( const auto & par : porIsla ) {
        std::string lista;
        for ( const auto & item : par.second ) {
            if ( !lista.empty() ) lista += ";";
            lista += item.nombre + "," + std::to_string( item.cantidad );
        }

        MensajeMancomunado resp;
        std::string isla = textoIsla( par.first );
        if ( !solicitarIsla( par.first, armarMancomunado( MAN_BUY, { std::to_string( par.second.size() ), lista } ),
                             "compra de lo reservado en " + isla, resp ) ) {
            continue;   // sin respuesta: esos productos quedan fuera de la factura
        }
        if ( resp.tipo != MAN_ACK || resp.campos.size() < 3 ) continue;

        total += std::stof( resp.campos[ 0 ] );
        for ( const auto & item : separar( resp.campos[ 2 ], ';' ) ) {
            ++cantidadItems;
            if ( !detalle.empty() ) detalle += ";";
            detalle += item;
        }
    }

    responderCliente( MSG_FACTURA, { formatearMonto( total ), std::to_string( cantidadItems ), detalle },
                      "factura proforma con lo reservado en las dos islas" );
    carrito_.clear();
}

// ---------------------------------------------------------------------------
// protocolo mancomunado, lado de quien pregunta
// ---------------------------------------------------------------------------

// broadcast dirigido: a las otras islas primero y al final el eco a esta misma
void Intermediario::enviarBroadcast( const std::string & texto ) {
    DatosRed datos = armarDatos( 0, miId_, texto );
    for ( int idIsla : idsIslas_ ) {
        if ( idIsla != miId_ ) buzon_.Enviar( &datos, sizeof( datos ), canalPuerto( idIsla ) );
    }
    buzon_.Enviar( &datos, sizeof( datos ), canalPuerto( miId_ ) );
}

// Una solicitud y una respuesta, como una conexion tcp al puerto 9090 de la otra isla.
// Si no llega respuesta se reintenta hasta INTENTOS_ISLA veces.
bool Intermediario::solicitarIsla( int idIsla, const std::string & texto, const std::string & explicacion,
                                    MensajeMancomunado & respuesta ) {
    DatosRed datos = armarDatos( canalRespuesta( miId_ ), miId_, texto );

    for ( int intento = 1; intento <= INTENTOS_ISLA; ++intento ) {
        // una respuesta que llego tarde a un intento anterior no debe tomarse como respuesta de este
        PaqueteRed paquete;
        while ( buzon_.RecibirConEspera( &paquete, sizeof( paquete.datos ), canalRespuesta( miId_ ), 1, 0 ) != -1 ) {
            rotular( idIsla, miId_, paquete.datos.texto, "(mancomunado) respuesta atrasada: se descarta" );
        }

        std::string nota = ( intento == 1 ) ? explicacion : "sin respuesta, intento " + std::to_string( intento );
        rotular( miId_, idIsla, texto, "(mancomunado) " + nota );
        buzon_.Enviar( &datos, sizeof( datos ), canalPuerto( idIsla ) );

        int st = buzon_.RecibirConEspera( &paquete, sizeof( paquete.datos ), canalRespuesta( miId_ ),
                                          SONDEOS_ISLA, ESPERA_ISLA_MS );
        if ( st == -1 ) continue;

        std::string linea( paquete.datos.texto );
        rotular( idIsla, miId_, linea, "(mancomunado) respuesta recibida" );
        try {
            respuesta = parsearMancomunado( linea );
        } catch ( const std::invalid_argument & ) {
            respuesta = { MAN_ERROR, { std::to_string( ERR_MAL_FORMADO ) } };
        }
        return true;
    }
    return false;
}

// Pide MAN_GET_PRODUCTS de cada categoria remota hasta encontrar el producto
bool Intermediario::buscarProductoRemoto( const std::string & producto ) {
    for ( const auto & par : islaDeCategoria_ ) {
        MensajeMancomunado resp;
        if ( !solicitarIsla( par.second, armarMancomunado( MAN_GET_PRODUCTS, { par.first } ),
                             "buscando " + producto + " en " + par.first, resp ) ) {
            continue;
        }
        if ( resp.tipo == MAN_ACK && resp.campos.size() >= 2 ) {
            recordarProductos( par.second, resp.campos[ 1 ] );
        }
        if ( productoRemoto_.count( producto ) ) return true;
    }
    return false;
}

// guarda la isla y el precio de cada producto de un listado producto,precio,stock;...
void Intermediario::recordarProductos( int idIsla, const std::string & lista ) {
    for ( const auto & item : separar( lista, ';' ) ) {
        std::vector<std::string> partes = separar( item, ',' );
        if ( partes.size() == 3 ) {
            productoRemoto_[ partes[ 0 ] ] = { idIsla, std::stof( partes[ 1 ] ) };
        }
    }
}

std::string Intermediario::textoIsla( int idIsla ) {
    std::lock_guard<std::mutex> lock( mtxIslas_ );
    auto it = islas_.find( idIsla );
    return ( it == islas_.end() ) ? "isla desconocida" : it->second.idIsla;
}

void Intermediario::probarIsla( int idIsla ) {
    struct Prueba { std::string texto; std::string explicacion; };
    std::vector<Prueba> pruebas = {
        { "10",               "categorias de la isla" },
        { "20/BebidasCalientes", "productos; la isla busca en minusculas" },
        { "20/libros",        "categoria que no existe: se espera 03" },
        { "20/bebidas#frias", "categoria con caracter invalido: se espera 02/102" },
        { "50/capuchino/2",   "reserva valida: se espera 01" },
        { "50/tarta/999",     "mas de lo que hay: se espera 02/103" },
        { "50/pastel/1",      "producto que no existe: se espera 03" },
        { "40/1/capuchino,2", "compra de lo reservado: se espera 01 con la factura" },
        { "40/1/menta,1",     "compra sin reserva: se espera 02/104" },
        { "77",               "tipo que no existe: se espera 02/101" },
        { "abc",              "tipo que no es de dos digitos: se espera 02/100" },
    };

    for ( const auto & prueba : pruebas ) {
        MensajeMancomunado resp;
        solicitarIsla( idIsla, prueba.texto, prueba.explicacion, resp );
    }
}

// ---------------------------------------------------------------------------
// protocolo mancomunado, lado de la isla que responde (hilo escucha_)
// ---------------------------------------------------------------------------

void Intermediario::escuchar() {
    while ( true ) {
        PaqueteRed paquete;
        try {
            buzon_.Recibir( &paquete, sizeof( paquete.datos ), canalPuerto( miId_ ) );
        } catch ( const std::runtime_error & ) {
            return;
        }

        std::string linea( paquete.datos.texto );
        int idOrigen = paquete.datos.idOrigen;

        MensajeMancomunado mm;
        try {
            mm = parsearMancomunado( linea );
        } catch ( const std::invalid_argument & ) {
            rotular( idOrigen, miId_, linea, "(mancomunado) solicitud recibida en el puerto 9090" );
            responderSolicitud( paquete.datos, { armarMancomunado( MAN_ERROR, { std::to_string( ERR_MAL_FORMADO ) } ),
                                                 "mensaje mal formado" } );
            continue;
        }

        if ( mm.tipo == MAN_ANNOUNCE ) {
            recibirAnuncio( mm, idOrigen, linea );
            continue;
        }
        if ( mm.tipo == MAN_ANNOUNCE_BYE ) {
            if ( recibirDespedida( mm, idOrigen, linea ) ) return;
            continue;
        }

        rotular( idOrigen, miId_, linea, "(mancomunado) solicitud recibida en el puerto 9090" );
        responderSolicitud( paquete.datos, atenderSolicitud( mm, idOrigen ) );
    }
}

void Intermediario::recibirAnuncio( const MensajeMancomunado & mm, int idOrigen, const std::string & linea ) {
    if ( mm.campos.size() != 4 || !validarIdMensaje( mm.campos[ 0 ] ) || !validarIdIsla( mm.campos[ 1 ] ) ||
         !validarIp( mm.campos[ 2 ] ) || !validarPuerto( mm.campos[ 3 ] ) ) {
        rotular( idOrigen, miId_, linea, "(mancomunado) anuncio invalido: se descarta" );   // por udp no se responde
        return;
    }

    const std::string & idIsla = mm.campos[ 1 ];
    if ( idIsla == idTexto_ ) {
        rotular( idOrigen, miId_, linea, "(mancomunado) eco de su propio anuncio: se descarta" );
        return;
    }

    std::string clave = idIsla + "/" + mm.campos[ 0 ];
    if ( anunciosVistos_.count( clave ) ) {
        rotular( idOrigen, miId_, linea, "(mancomunado) anuncio duplicado: se descarta" );
        return;
    }
    anunciosVistos_.insert( clave );

    {
        std::lock_guard<std::mutex> lock( mtxIslas_ );
        islas_[ idOrigen ] = { idIsla, mm.campos[ 2 ], mm.campos[ 3 ] };
    }
    cvIslas_.notify_all();
    rotular( idOrigen, miId_, linea, "(mancomunado) isla " + idIsla + " registrada en " + mm.campos[ 2 ] );
}

// devuelve true cuando llega el eco de su propia despedida, que es la senal para terminar la escucha
bool Intermediario::recibirDespedida( const MensajeMancomunado & mm, int idOrigen, const std::string & linea ) {
    if ( mm.campos.size() != 1 || !validarIdIsla( mm.campos[ 0 ] ) ) {
        rotular( idOrigen, miId_, linea, "(mancomunado) despedida invalida: se descarta" );
        return false;
    }

    if ( mm.campos[ 0 ] == idTexto_ ) {
        rotular( idOrigen, miId_, linea, "(mancomunado) eco de su propia despedida: termina la escucha" );
        return true;
    }

    {
        std::lock_guard<std::mutex> lock( mtxIslas_ );
        for ( auto it = islas_.begin(); it != islas_.end(); ++it ) {
            if ( it->second.idIsla == mm.campos[ 0 ] ) {
                islas_.erase( it );
                break;
            }
        }
    }
    rotular( idOrigen, miId_, linea, "(mancomunado) isla " + mm.campos[ 0 ] + " dada de baja" );
    return false;
}

RespuestaIsla Intermediario::atenderSolicitud( const MensajeMancomunado & mm, int idOrigen ) {
    switch ( mm.tipo ) {
        case MAN_GET_CATEGORIES: return atenderCategoriasRemotas();
        case MAN_GET_PRODUCTS:   return atenderProductosRemotos( mm.campos );
        case MAN_RESERVE:        return atenderReservaRemota( mm.campos, idOrigen );
        case MAN_BUY:            return atenderCompraRemota( mm.campos, idOrigen );
        default:
            return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_TIPO_NO_SOPORTADO ) } ), "tipo no soportado" };
    }
}

// 10 -> 01/categoria1,categoria2,... con las categorias de todas las bodegas propias, en minusculas
RespuestaIsla Intermediario::atenderCategoriasRemotas() {
    std::string lista;
    for ( int idBodega : idsBodega_ ) {
        MensajeProtocolo resp;
        if ( !consultarBodega( idBodega, MSG_REQ_CATEGORIES, {}, "categorias para otra isla", resp ) ) continue;
        if ( resp.tipo != MSG_CATEGORY_LIST || resp.campos.empty() ) continue;

        for ( const auto & categoria : separar( resp.campos[ 0 ], ',' ) ) {
            if ( !lista.empty() ) lista += ",";
            lista += minusculas( categoria );
        }
    }
    return { armarMancomunado( MAN_ACK, { lista } ), "categorias de " + idTexto_ };
}

// 20/categoria -> 01/cantidad/producto,precio,stock;... o 03/categoria si no existe
RespuestaIsla Intermediario::atenderProductosRemotos( const std::vector<std::string> & campos ) {
    if ( campos.empty() || !validarCategoriaMancomunado( campos[ 0 ] ) ) {
        return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_VALOR_INVALIDO ) } ), "categoria no cumple regex" };
    }

    // la isla convierte a minusculas antes de buscar
    std::string buscada = minusculas( campos[ 0 ] );
    for ( const auto & par : categoriaBodega_ ) {
        if ( minusculas( par.first ) != buscada ) continue;

        MensajeProtocolo resp;
        if ( !consultarBodega( par.second, MSG_REQ_PRODUCTS, { par.first }, "productos para otra isla", resp ) ) {
            return { "", "la bodega no respondio: no se contesta y la otra isla reintenta" };
        }
        if ( resp.tipo != MSG_PRODUCT_LIST || resp.campos.empty() || resp.campos[ 0 ] == "0" ) {
            return { armarMancomunado( MAN_ACK, { "0" } ), "categoria sin productos" };
        }
        return { armarMancomunado( MAN_ACK, { resp.campos[ 0 ], resp.campos.size() > 1 ? resp.campos[ 1 ] : "" } ),
                 "productos de " + par.first };
    }
    return { armarMancomunado( MAN_NOT_FOUND, { campos[ 0 ] } ), "categoria inexistente en " + idTexto_ };
}

// 50/producto/cantidad -> 01, 02/103 si no alcanza, 03/producto si no existe.
// El mancomunado no tiene reservas parciales, por eso se revisa el stock antes del 50 interno.
RespuestaIsla Intermediario::atenderReservaRemota( const std::vector<std::string> & campos, int idOrigen ) {
    if ( campos.size() < 2 || !validarProductoMancomunado( campos[ 0 ] ) ||
         !validarCantidadMancomunado( campos[ 1 ] ) || std::stoi( campos[ 1 ] ) == 0 ) {
        return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_VALOR_INVALIDO ) } ), "producto o cantidad invalidos" };
    }

    std::string nombreMinusculas = minusculas( campos[ 0 ] );
    int pedido = std::stoi( campos[ 1 ] );

    int idBodega;
    std::string nombre;
    float precio;
    int stock;
    if ( !buscarProductoLocal( nombreMinusculas, idBodega, nombre, precio, stock ) ) {
        return { armarMancomunado( MAN_NOT_FOUND, { campos[ 0 ] } ), "producto inexistente en " + idTexto_ };
    }
    if ( stock < pedido ) {
        return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_STOCK ) } ),
                 "stock insuficiente: hay " + std::to_string( stock ) };
    }

    MensajeProtocolo resp;
    if ( !consultarBodega( idBodega, MSG_RESERVE_STOCK, { nombre, std::to_string( pedido ) },
                           "reserva para otra isla", resp ) ) {
        return { "", "la bodega no respondio: no se contesta y la otra isla reintenta" };
    }
    if ( resp.tipo != MSG_RESERVED || resp.campos.size() < 3 ) {
        return { armarMancomunado( MAN_NOT_FOUND, { campos[ 0 ] } ), "la bodega no tiene el producto" };
    }

    int reservado = std::stoi( resp.campos[ 2 ] );
    Reserva & reserva = reservas_[ idOrigen ][ nombreMinusculas ];
    reserva.cantidad += reservado;
    reserva.precio = precio;
    reserva.hora = std::chrono::steady_clock::now();

    // reservado solo es menor si otro cliente compro entre la revision y la reserva
    if ( reservado < pedido ) {
        return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_STOCK ) } ), "stock insuficiente al reservar" };
    }
    return { armarMancomunado( MAN_ACK, {} ), "reservado por " + std::to_string( SEGUNDOS_VIGENCIA_RESERVA ) + " s" };
}

// 40/n/producto,cantidad;... -> 01/total/n/producto,cantidad,subtotal;...
// Cada producto necesita una reserva vigente de quien compra; si alguno no la tiene se responde 02/104.
RespuestaIsla Intermediario::atenderCompraRemota( const std::vector<std::string> & campos, int idOrigen ) {
    RespuestaIsla invalido = { armarMancomunado( MAN_ERROR, { std::to_string( ERR_VALOR_INVALIDO ) } ),
                               "compra con formato invalido" };
    if ( campos.size() < 2 || !validarCount( campos[ 0 ] ) ) return invalido;

    struct Linea { std::string nombre; int cantidad; };
    std::vector<Linea> lineas;
    for ( const auto & item : separar( campos[ 1 ], ';' ) ) {
        std::vector<std::string> partes = separar( item, ',' );
        if ( partes.size() != 2 || !validarProductoMancomunado( partes[ 0 ] ) ||
             !validarCantidadMancomunado( partes[ 1 ] ) || std::stoi( partes[ 1 ] ) == 0 ) {
            return invalido;
        }
        lineas.push_back( { minusculas( partes[ 0 ] ), std::stoi( partes[ 1 ] ) } );
    }
    if ( (int) lineas.size() != std::stoi( campos[ 0 ] ) ) return invalido;

    auto & misReservas = reservas_[ idOrigen ];
    auto ahora = std::chrono::steady_clock::now();
    for ( const auto & linea : lineas ) {
        auto it = misReservas.find( linea.nombre );
        if ( it == misReservas.end() || it->second.cantidad < linea.cantidad ||
             ahora - it->second.hora > std::chrono::seconds( SEGUNDOS_VIGENCIA_RESERVA ) ) {
            return { armarMancomunado( MAN_ERROR, { std::to_string( ERR_SIN_RESERVA ) } ),
                     "sin reserva vigente de " + linea.nombre };
        }
    }

    float total = 0.0f;
    std::string detalle;
    for ( const auto & linea : lineas ) {
        Reserva & reserva = misReservas[ linea.nombre ];
        float subtotal = reserva.precio * linea.cantidad;
        total += subtotal;
        reserva.cantidad -= linea.cantidad;
        if ( reserva.cantidad == 0 ) misReservas.erase( linea.nombre );

        if ( !detalle.empty() ) detalle += ";";
        detalle += linea.nombre + "," + std::to_string( linea.cantidad ) + "," + formatearMonto( subtotal );
    }
    return { armarMancomunado( MAN_ACK, { formatearMonto( total ), std::to_string( lineas.size() ), detalle } ),
             "factura de " + idTexto_ };
}

// la respuesta va por la misma "conexion": el canal que mando quien pregunto
void Intermediario::responderSolicitud( const DatosRed & solicitud, const RespuestaIsla & respuesta ) {
    if ( solicitud.canalRespuesta == 0 ) return;   // por broadcast no se responde
    if ( respuesta.texto.empty() ) {
        rotular( miId_, solicitud.idOrigen, "(sin respuesta)", "(mancomunado) " + respuesta.explicacion );
        return;
    }

    rotular( miId_, solicitud.idOrigen, respuesta.texto, "(mancomunado) " + respuesta.explicacion );
    DatosRed datos = armarDatos( 0, miId_, respuesta.texto );
    buzon_.Enviar( &datos, sizeof( datos ), solicitud.canalRespuesta );
}
