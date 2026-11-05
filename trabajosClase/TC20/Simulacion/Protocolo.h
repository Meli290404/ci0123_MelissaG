#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>

// Identificador numerico interno de cada componente, usado solo para
// calcular el canal de System V (mtype). No es lo mismo que el id de
// texto que viaja dentro del mensaje (ver ID_TXT_BODEGA mas abajo).
const int ID_CLIENTE   = 1;  // cliente de la isla 4
const int ID_INT04     = 2;  // intermediario de la isla 4
const int ID_BODEGA1   = 3;  // isla 4: BebidasFrias, BebidasCalientes, Postres
const int ID_BODEGA2   = 4;  // isla 4: Caramelos, Reposteria
const int ID_INT05     = 5;  // intermediario de la isla 5
const int ID_BODEGA3   = 6;  // isla 5: helados, snacks
const int ID_BROADCAST = 9;  // solo se usa en los rotulos de los mensajes por broadcast

// canal del protocolo interno entre dos componentes de la misma isla
inline long canal( int origen, int destino ) {
    return origen * 10 + destino;
}

// canal donde las bodegas de un intermediario se anuncian al arrancar
inline long canalRegistro( int idIntermediario ) {
    return 90 + idIntermediario;
}

const int TAM_MAX_MENSAJE = 256;

// Ids de texto que viajan dentro del mensaje (campos origen y destino).
// Las bodegas de una isla comparten ID_TXT_BODEGA; se distinguen por canal
// y por el id numerico que cada una manda en su mensaje MSG_ANNOUNCE.
const std::string ID_TXT_CLIENTE = "CLI_04";
const std::string ID_TXT_INT04   = "INT_04";
const std::string ID_TXT_INT05   = "INT_05";
const std::string ID_TXT_BODEGA  = "SERV";

// Codigos de tipo de mensaje, seccion a (cliente e intermediario)
// y seccion b (intermediario y bodega) del protocolo interno de la isla.
const int MSG_REQ_CATEGORIES  = 10; // Cliente -> Intermediario y Intermediario -> Bodega: pide categorias
const int MSG_CATEGORY_LIST   = 11; // Bodega -> Intermediario y Intermediario -> Cliente: lista de categorias
const int MSG_REQ_PRODUCTS    = 20; // Cliente -> Intermediario y Intermediario -> Bodega: productos de una categoria
const int MSG_PRODUCT_LIST    = 22; // Bodega -> Intermediario y Intermediario -> Cliente: count/lista
const int MSG_NO_PRODUCTS     = 23; // Intermediario -> Cliente: categoria sin productos
const int MSG_PROD_NOT_FOUND  = 27; // Bodega -> Intermediario: el producto no esta en esa bodega
const int MSG_PROD_NOT_AVAIL  = 28; // Intermediario -> Cliente: ninguna bodega ni isla tiene el producto
const int MSG_ADD_CART        = 30; // Cliente -> Intermediario: agregar al carrito
const int MSG_CART_ADDED      = 32; // Intermediario -> Cliente: producto/precio/cantidad reservada
const int MSG_REQUEST_FACTURA = 40; // Cliente -> Intermediario: count/lista del carrito
const int MSG_FACTURA         = 41; // Intermediario -> Cliente: total/count/detalle
const int MSG_FACT_VACIA      = 42; // Intermediario -> Cliente: carrito vacio
const int MSG_RESERVE_STOCK   = 50; // Intermediario -> Bodega: producto/cantidad
const int MSG_RESERVED        = 51; // Bodega -> Intermediario: producto/precio/cantidad reservada
const int MSG_ANNOUNCE        = 60; // Bodega -> Intermediario: registro con categorias
const int MSG_DEATH           = 61; // cierre ordenado
const int MSG_ERR_FORMAT      = 90; // Detector -> Emisor: campo no cumple regex
const int MSG_ERR_SIZE        = 91; // Detector -> Emisor: campo mas largo que su maximo
const int MSG_ERR_COMM        = 92; // Intermediario -> Cliente: una bodega u otra isla no respondio

// Un producto de bodega con su categoria, porque una misma
// bodega puede manejar varias categorias a la vez.
struct Producto {
    std::string categoria;
    std::string nombre;
    float precio;
    int stock;
    std::string descripcion;
};

// Item ya reservado, en una bodega propia (idIsla en 0) o en otra isla (idIsla de su intermediario)
struct ItemCarrito {
    std::string nombre;
    float precio;
    int cantidad;
    int idIsla;
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
