#ifndef DESCUBRIMIENTO_H
#define DESCUBRIMIENTO_H

#include <string>
#include <vector>
#include "Socket.h"
#include "Protocolo.h"

// funciones que comparten el intermediario y el servidor de productos para el descubrimiento
// por broadcast (mensajes 80 a 83). todo va por udp con sendTo y recvFrom, porque al inicio
// ninguno conoce la direccion del otro y no se puede abrir una conexion tcp

// direccion de broadcast de la red a la que pertenece esa ip (172.16.123.67 -> 172.16.123.79),
// "" si la ip no es de ninguna interfaz de esta computadora
std::string broadcastDe(const std::string& ip);

// manda un datagrama a ip:puerto
void enviarDatagrama(Socket& udp, const std::string& mensaje, const std::string& ip, int puerto);

// espera un datagrama (recvFrom); deja en ip y puerto quien lo mando
std::string recibirDatagrama(Socket& udp, std::string& ip, int& puerto);

// campos del anuncio de un servidor (80 y 83): <puerto tcp>/<count>/<cat,cat,...>
std::vector<std::string> camposAnuncio(int puerto, const std::vector<std::string>& categorias);

// lee y valida el anuncio de un servidor; false si algun campo no es valido
bool leerAnuncio(const Mensaje& msg, int& puerto, std::vector<std::string>& categorias);

#endif
