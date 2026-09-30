#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>

// Identificador numerico interno de cada componente, usado SOLO para
// calcular el canal de System V (mtype). No es lo mismo que el ID de
// texto que viaja dentro del mensaje del protocolo (ver mas abajo),
// ya que el protocolo exige "SERV" literal para cualquier bodega.
const int ID_CLIENTE       = 1;
const int ID_INTERMEDIARIO = 2;
const int ID_BODEGA1       = 3;  // BebidasFrias, BebidasCalientes, Postres
const int ID_BODEGA2       = 4;  // Caramelos, Reposteria

const int NUM_BODEGAS = 2;

inline long canal( int origen, int destino ) {
    return origen * 10 + destino;
}

// Canal compartido que usan las dos bodegas para anunciarse al arrancar.
const long CANAL_REGISTRO = 99;

const int TAM_MAX_MENSAJE = 256;

// IDs de texto que viajan dentro del mensaje (campos ORIGEN/DESTINO).
// El regex del protocolo exige "SERV" literal sin sufijo, asi que las
// dos bodegas comparten el mismo ID de texto; se distinguen por canal
// y por el id numerico que cada una manda en el cuerpo de su ANNOUNCE.
const std::string ID_TXT_CLIENTE       = "CLI_04";
const std::string ID_TXT_INTERMEDIARIO = "INT_04";
const std::string ID_TXT_BODEGA        = "SERV";

// Codigos de tipo de mensaje (campo TIPO_MENSAJE del protocolo).
const int MSG_REQ_CAT_CLI     = 10; // Cliente -> Intermediario: pide productos de una categoria
const int MSG_REQ_PROD_CLI    = 20; // Cliente -> Intermediario: pide detalle de un producto
const int MSG_REQ_CAT_BOD     = 21; // Intermediario -> Bodega: reenvia la solicitud de categoria
const int MSG_PROD_LIST       = 22; // Bodega -> Intermediario, Intermediario -> Cliente: listado
const int MSG_PROD_LIST_EMPT  = 23; // Intermediario -> Cliente: categoria sin productos
const int MSG_REQ_PROD_BOD    = 24; // Intermediario -> Bodega: pide detalle (consulta o carrito)
const int MSG_PROD_FOUND      = 25; // Bodega -> Intermediario: detalle encontrado
const int MSG_PROD_DETAIL     = 26; // Intermediario -> Cliente: detalle de producto
const int MSG_PROD_NOT_FOUND  = 27; // Bodega -> Intermediario: no existe
const int MSG_PROD_NOT_AVAIL  = 28; // Intermediario -> Cliente: no disponible
const int MSG_ADD_CART        = 30; // Cliente -> Intermediario: agregar al carrito
const int MSG_PROD_ADDED      = 32; // Intermediario -> Cliente: confirmacion de agregado
const int MSG_ERR_STOCK       = 33; // Intermediario -> Cliente: stock insuficiente
const int MSG_REQUEST_FACTURA = 40; // Cliente -> Intermediario: solicita factura
const int MSG_FACTURA         = 41; // Intermediario -> Cliente: factura proforma
const int MSG_FACT_VACIA      = 42; // Intermediario -> Cliente: carrito vacio
const int MSG_ANNOUNCE        = 60; // Bodega -> Intermediario: registro con categorias
const int MSG_DEATH           = 61; // cierre ordenado (reemplaza el EXIT anterior)
const int MSG_ERR_FORMAT      = 90; // Detector -> Emisor: campo no cumple regex
const int MSG_ERR_SIZE        = 91; // reservado, no se emite en esta simulacion
const int MSG_ERR_COMM        = 92; // Intermediario -> Cliente: no hubo respuesta

// Un producto de bodega. Ahora incluye su categoria porque una misma
// bodega puede manejar varias categorias a la vez.
struct Producto {
    std::string categoria;
    std::string nombre;
    float precio;
    int stock;
    std::string descripcion;
};

// Item ya confirmado en el carrito via ADD_CART
struct ItemCarrito {
    std::string nombre;
    float precio;
    int cantidad;
};

// Un mensaje ya separado en sus componentes, resultado de parsearMensaje().
struct MensajeProtocolo {
    std::string origen;
    std::string destino;
    int tipo;
    std::vector<std::string> campos;
};

// Arma una linea con el formato ORIGEN|DESTINO/TIPO/campo1/campo2/...
std::string armarMensaje( const std::string & origen, const std::string & destino,
                           int tipo, const std::vector<std::string> & campos );

// Separa una linea recibida en sus componentes. Si la linea no tiene
// el formato esperado (falta el '|' o el tipo no es numerico), lanza
// std::invalid_argument.
MensajeProtocolo parsearMensaje( const std::string & linea );

#endif