#ifndef ATENCIONCLIENTE_H
#define ATENCIONCLIENTE_H

#include <string>
#include <vector>
#include "HttpUtil.h"
#include "Protocolo.h"
#include "TablaRutas.h"
#include "TablaIslas.h"
#include <map>
#include "Carritos.h"
#include "PaginaWeb.h"

// logica del intermediario: recibe la peticion http del cliente, consulta a los servidores de
// productos de la tabla de rutas con el protocolo interno y arma la respuesta html.
// no maneja el socket del cliente
class AtencionCliente {
public:
    AtencionCliente(TablaRutas& rutas, TablaIslas& islas, Carritos& carritos);

    RespuestaHttp responder(const std::string& peticion);

private:
    TablaRutas& rutas_;
    TablaIslas& islas_;
    Carritos& carritos_;
    PaginaWeb pagina_;

    // pagina de inicio con un boton por pagina
    RespuestaHttp menu();

    // todos los productos de la isla y de las otras islas, para el navegador
    RespuestaHttp inventario();
    RespuestaHttp sesion();
    RespuestaHttp categorias();
    RespuestaHttp productos(const std::string& categoria);
    RespuestaHttp agregar(const std::string& sesionTexto, const std::string& producto, const std::string& cantidadTexto);
    RespuestaHttp factura(const std::string& sesionTexto);

    // tabla de rutas (servidores de la isla y otras islas) para el navegador; se recarga sola cada 2 segundos
    RespuestaHttp paginaRutas();

    // solo los servidores o solo las otras islas; tambien se recargan solas
    RespuestaHttp paginaServidores();
    RespuestaHttp paginaIslas();

    // partes de las paginas anteriores. conTitulo pone el subtitulo, que sobra en la pagina que
    // solo tiene esa tabla porque ya lleva el mismo titulo arriba
    std::string tablaServidores(const std::vector<ServidorRuta>& servidores, bool conTitulo);
    std::string tablaRutasCategorias(const std::vector<ServidorRuta>& servidores, int& filas);
    std::string tablaIslas(const std::vector<IslaRuta>& islas, bool conTitulo);

    // envia un mensaje a un servidor de productos y deja la respuesta ya parseada.
    // si no contesta, lo marca sin respuesta y no se usa hasta su proximo 83 (o 80)
    bool consultar(const ServidorRuta& servidor, int tipo, const std::vector<std::string>& campos,
                   Mensaje& respuesta);

    // agrega al carrito un producto de otra isla, reservandolo alla con el 50 del mancomunado.
    // devuelve true si armo la respuesta; contesto queda en true si alguna isla lo tenia
    bool agregarDeIsla(int sesionId, const std::string& producto, int pedido, bool& contesto,
                       RespuestaHttp& resultado);

    // compra con el 40 del mancomunado lo que el carrito tiene reservado en esa isla
    bool comprarEnIsla(const std::string& idIsla, const std::vector<ItemReservado>& items,
                       std::map<std::string, uint64_t>& subtotales);

    // respuesta para cuando la tabla de rutas no tiene ningun servidor activo
    RespuestaHttp sinServidores();

    // respuesta para cuando habia servidores pero ninguno contesto
    RespuestaHttp sinComunicacion();

    // categorias de un servidor (mensaje 10); vacio si no contesto
    std::vector<CategoriaVista> categoriasDe(const ServidorRuta& servidor, bool& contesto);

    // convierte la lista de un mensaje 22 en vistas; devuelve false si la lista viene mal formada
    bool leerListaProductos(const Mensaje& mensaje, std::vector<ProductoVista>& productos);

    // convierte la lista de un mensaje 11 (id:nombre,id:nombre) en vistas sin productos
    std::vector<CategoriaVista> leerListaCategorias(const Mensaje& mensaje);

    bool leerSesion(const std::string& sesionTexto, int& sesion);

    RespuestaHttp respuesta(int codigo, const std::string& titulo, const std::string& estado,
                            const std::string& atributos, const std::string& contenido);
    RespuestaHttp error(int codigo, const std::string& estado, const std::string& detalle);
};

#endif