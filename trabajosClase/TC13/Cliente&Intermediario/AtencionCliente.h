#ifndef ATENCIONCLIENTE_H
#define ATENCIONCLIENTE_H

#include <string>
#include <vector>
#include "HttpUtil.h"
#include "Protocolo.h"
#include "ConexionBodega.h"
#include "Carritos.h"
#include "PaginaWeb.h"

// logica del intermediario: recibe la peticion http del cliente, consulta al servidor de
// productos con el protocolo interno y arma la respuesta html. no maneja el socket del cliente
class AtencionCliente {
public:
    AtencionCliente(ConexionBodega& bodega, Carritos& carritos);

    RespuestaHttp responder(const std::string& peticion);

private:
    ConexionBodega& bodega_;
    Carritos& carritos_;
    PaginaWeb pagina_;

    RespuestaHttp inicio();
    RespuestaHttp sesion();
    RespuestaHttp categorias();
    RespuestaHttp productos(const std::string& categoria);
    RespuestaHttp agregar(const std::string& sesionTexto, const std::string& producto, const std::string& cantidadTexto);
    RespuestaHttp factura(const std::string& sesionTexto);

    // envia un mensaje al servidor de productos y deja la respuesta ya parseada.
    // si devuelve false, error trae la respuesta http que se le da al cliente
    bool consultar(int tipo, const std::vector<std::string>& campos, Mensaje& respuesta, RespuestaHttp& error);

    // convierte la lista de un mensaje 22 en vistas; devuelve false si la lista viene mal formada
    bool leerListaProductos(const Mensaje& mensaje, std::vector<ProductoVista>& productos);

    bool leerSesion(const std::string& sesionTexto, int& sesion);

    RespuestaHttp respuesta(int codigo, const std::string& titulo, const std::string& estado,
                            const std::string& atributos, const std::string& contenido);
    RespuestaHttp error(int codigo, const std::string& estado, const std::string& detalle);
};

#endif