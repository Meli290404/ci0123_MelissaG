#ifndef MANCOMUNADO_H
#define MANCOMUNADO_H

#include <string>
#include <vector>

// protocolo mancomunado del grupo 2, entre intermediarios de islas distintas: TIPO/campo1/campo2/...
// no lleva origen ni destino: en tcp los da el socket y en udp el origen es el id_isla del 60.
// el mismo numero de puerto sirve para udp (60 y 61) y para tcp (10, 20, 50, 40 y respuestas)
const int PUERTO_MANCOMUNADO = 9090;

const int MAN_ACK            = 1;    // 01/<datos pedidos>
const int MAN_ERROR          = 2;    // 02/<codigo>
const int MAN_NOT_FOUND      = 3;    // 03/<valor>: la categoria o el producto no existe en la isla
const int MAN_GET_CATEGORIES = 10;   // 10
const int MAN_GET_PRODUCTS   = 20;   // 20/<categoria>
const int MAN_BUY            = 40;   // 40/<n>/<producto,cantidad;...>
const int MAN_RESERVE        = 50;   // 50/<producto>/<cantidad>
const int MAN_ANNOUNCE       = 60;   // 60/<ID_MSG>/<id_isla>/<ip>/<puerto>, udp al broadcast de cada isla
const int MAN_ANNOUNCE_BYE   = 61;   // 61/<id_isla>, udp al broadcast de cada isla

// codigos que acompanan a MAN_ERROR
const int ERR_MAL_FORMADO       = 100;
const int ERR_TIPO_NO_SOPORTADO = 101;
const int ERR_VALOR_INVALIDO    = 102;
const int ERR_STOCK             = 103;
const int ERR_SIN_RESERVA       = 104;

// broadcast dirigido de cada isla del laboratorio (redes /28 dentro de 172.16.123.0)
const std::vector<std::string> BROADCASTS_ISLAS = {
    "172.16.123.15",    // isla profes
    "172.16.123.31",    // isla 1
    "172.16.123.47",    // isla 2
    "172.16.123.63",    // isla 3
    "172.16.123.79",    // isla 4
    "172.16.123.95",    // isla 5
    "172.16.123.111"    // isla 6
};

struct MensajeMancomunado {
    int tipo;
    std::vector<std::string> campos;
};

// arma una linea TIPO/campo1/campo2/... con el tipo en dos digitos
std::string armarMancomunado(int tipo, const std::vector<std::string>& campos);

// separa una linea recibida. si el tipo no son dos digitos lanza std::invalid_argument,
// que en tcp se contesta con 02/100
MensajeMancomunado parsearMancomunado(const std::string& linea);

// validaciones con las expresiones regulares del documento del protocolo mancomunado
bool validarIdMensaje(const std::string& s);           // ^[0-9]{4}$
bool validarIdIsla(const std::string& s);              // ^INT_[0-9]{2}$
bool validarPuertoMancomunado(const std::string& s);   // ^[0-9]{1,5}$, de 1 a 65535
bool validarNombreMancomunado(const std::string& s);   // categoria y producto: ^[a-zA-Z0-9_\-]{1,50}$
bool validarCantidadMancomunado(const std::string& s); // ^[0-9]{1,5}$
bool validarCountMancomunado(const std::string& s);    // ^[0-9]{1,3}$

// el mancomunado busca sin importar mayusculas: la isla convierte a minusculas antes de buscar
std::string aMinusculas(std::string s);

#endif
