#ifndef CONSULTASERVIDOR_H
#define CONSULTASERVIDOR_H

#include <string>
#include <vector>
#include "Protocolo.h"
#include "TablaRutas.h"

// envia un mensaje del protocolo interno a un servidor de productos y deja la respuesta ya parseada.
// la usan AtencionCliente (clientes de la isla) y AtencionIsla (otras islas).
// si el servidor no contesta, lo marca sin respuesta en la tabla de rutas
bool consultarServidor(TablaRutas& rutas, const ServidorRuta& servidor, int tipo,
                       const std::vector<std::string>& campos, Mensaje& respuesta);

#endif
