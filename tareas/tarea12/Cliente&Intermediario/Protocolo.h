#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>
#include <cstdint>

const int TAM_MAX_MENSAJE = 256;

const std::string ID_SERVIDOR = "SERV";
const std::string ID_INTERMEDIARIO = "INT_04";

// intermediario y servidor de productos
const int TIPO_REQUEST_CATEGORIES = 10;
const int TIPO_CATEGORY_LIST      = 11;
const int TIPO_REQUEST_PRODUCTS   = 20;
const int TIPO_PRODUCT_LIST       = 22;
const int TIPO_PRODUCT_NOT_FOUND  = 27;
const int TIPO_RESERVE_STOCK      = 50;
const int TIPO_RESERVED           = 51;
const int TIPO_ERR_FORMATO        = 90;
const int TIPO_ERR_TAMANO         = 91;
const int TIPO_ERR_COMM           = 92;

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

// precio en centavos a texto con dos decimales, por ejemplo 150000 a "1500.00"
std::string precioATexto(uint32_t centavos);

#endif