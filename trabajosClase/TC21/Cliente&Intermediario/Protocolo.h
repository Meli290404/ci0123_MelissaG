#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>
#include <cstdint>

// el protocolo no tiene tamano maximo de mensaje; MAX_LINEA solo evita llenar la memoria
// si llega una linea que nunca termina en '\n'
const size_t MAX_LINEA = 65536;

const std::string ID_INTERMEDIARIO = "INT_04";
const std::string ID_BROADCAST = "BROADCAST";   // destino de los mensajes 80 y 82

// cada servidor de productos tiene un numero NN (01 a 99): su id es SERV_NN y
// escucha al intermediario por tcp en PUERTO_BASE_SERVIDORES + NN (SERV_01 -> 9051)
const int PUERTO_BASE_SERVIDORES = 9050;
std::string idServidor(int numero);
int puertoServidor(int numero);

// descubrimiento por broadcast (udp). no se conoce la direccion del otro lado,
// por eso no se puede usar tcp: se manda un datagrama a la direccion de broadcast
// de la isla (isla 4: 172.16.123.79) y la respuesta llega directo a quien pregunto
const int PUERTO_UDP_INTERMEDIARIO = 9091;   // aqui escucha el intermediario (80 y 83)
const int PUERTO_UDP_SERVIDORES    = 9092;   // aqui escuchan los servidores de productos (81 y 82)

// intermediario y servidor de productos
const int TIPO_REQUEST_CATEGORIES = 10;
const int TIPO_CATEGORY_LIST      = 11;
const int TIPO_REQUEST_PRODUCTS   = 20;
const int TIPO_PRODUCT_LIST       = 22;
const int TIPO_PRODUCT_NOT_FOUND  = 27;
const int TIPO_RESERVE_STOCK      = 50;
const int TIPO_RESERVED           = 51;

// descubrimiento, por udp
const int TIPO_BUSCO_INTERMEDIARIO = 80;  // SERV_NN|BROADCAST/80/<puerto tcp>/<count>/<cat,cat,...>
const int TIPO_SOY_INTERMEDIARIO   = 81;  // INT_04|SERV_NN/81/<nuevo|actualizado>
const int TIPO_BUSCO_SERVIDORES    = 82;  // INT_04|BROADCAST/82
const int TIPO_SOY_SERVIDOR        = 83;  // SERV_NN|INT_04/83/<puerto tcp>/<count>/<cat,cat,...>

// el servidor de productos se apaga (udp al 9091 del intermediario)
const int TIPO_BAJA_SERVIDOR       = 72;  // SERV_NN|INT_04/72

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