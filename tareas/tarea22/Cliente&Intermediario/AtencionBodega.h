#ifndef ATENCIONBODEGA_H
#define ATENCIONBODEGA_H

#include <string>
#include "Protocolo.h"
#include "AdministradorBodegas.h"

// procesa un mensaje del intermediario y arma la respuesta del servidor de productos.
// no maneja sockets: recibe el texto del mensaje y devuelve el texto de la respuesta
class AtencionBodega {
public:
    // idServidor es el id de este servidor de productos, por ejemplo SERV_01
    AtencionBodega(AdministradorBodegas& admin, const std::string& idServidor);

    std::string responder(const std::string& texto);

private:
    AdministradorBodegas& admin_;
    std::string idServidor_;

    std::string listaCategorias(const std::string& destino);
    std::string listaProductos(const Mensaje& msg, const std::string& destino);
    std::string reservar(const Mensaje& msg, const std::string& destino);
    std::string devolver(const Mensaje& msg, const std::string& destino);
    std::string errorFormato(const std::string& destino, const std::string& tipoOriginal,
                             const std::string& campo, const std::string& valor);
};

#endif