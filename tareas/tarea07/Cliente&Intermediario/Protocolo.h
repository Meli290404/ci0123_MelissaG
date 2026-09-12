#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>

const int TAM_MAX_MENSAJE = 256;

// tipos cliente-intermediario
const int TIPO_REQUEST_CATEGORY = 10;
const int TIPO_CATEGORY_LIST    = 11;
const int TIPO_REQUEST_PRODUCTS = 20;
const int TIPO_NO_PRODUCTS      = 23;   // categoria sin productos (o inexistente)
const int TIPO_REQUEST_DETAIL   = 24;   // detalle de un producto por nombre
const int TIPO_PRODUCT_DETAIL   = 25;
const int TIPO_PRODUCT_LIST     = 26;
const int TIPO_PRODUCT_NOT_FOUND = 28;
const int TIPO_ADD_TO_CART      = 30;
const int TIPO_CART_ADDED       = 32;
const int TIPO_NO_STOCK         = 33;
const int TIPO_REQUEST_FACTURA  = 40;
const int TIPO_FACTURA          = 41;
const int TIPO_CARRITO_VACIO    = 42;
const int TIPO_ERR_FORMATO      = 90;
const int TIPO_ERR_TAMANO       = 91;
const int TIPO_ERR_COMM         = 92;

struct Mensaje {
    std::string origen;
    std::string destino;
    int tipo;
    std::vector<std::string> campos;
};

struct ItemCarrito {
    std::string producto;
    int cantidad;
    double precio;
};

// arma ORIGEN|DESTINO/TIPO/campo1/campo2/...
std::string construirMensaje(const std::string& origen, const std::string& destino,
                              int tipo, const std::vector<std::string>& campos);

// parsea texto crudo a Mensaje. lanza runtime_error si el formato basico esta roto
Mensaje parsearMensaje(const std::string& texto);

#endif