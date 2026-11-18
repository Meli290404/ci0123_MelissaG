#ifndef ATENCIONISLA_H
#define ATENCIONISLA_H

#include <string>
#include <vector>
#include "Mancomunado.h"
#include "TablaRutas.h"
#include "Reservas.h"

// logica del intermediario para las otras islas: recibe un mensaje del protocolo mancomunado
// que llego por el puerto tcp 9090, lo traduce al protocolo interno hacia los servidores de
// productos de la isla y arma la respuesta 01, 02 o 03. no maneja el socket
class AtencionIsla {
public:
    AtencionIsla(TablaRutas& rutas, Reservas& reservas);

    // linea es el mensaje tal como llego; ipIsla es la ip de la conexion, que identifica a la isla
    std::string responder(const std::string& linea, const std::string& ipIsla);

private:
    TablaRutas& rutas_;
    Reservas& reservas_;

    std::string categorias();                                                   // 10
    std::string productos(const MensajeMancomunado& msg);                       // 20
    std::string reservar(const MensajeMancomunado& msg, const std::string& ipIsla);   // 50
    std::string comprar(const MensajeMancomunado& msg, const std::string& ipIsla);    // 40

    // un producto encontrado en algun servidor de la isla
    struct ProductoIsla {
        ServidorRuta servidor;
        std::string nombre;       // como esta en el archivo de la bodega
        int stock;
    };

    // busca la categoria sin importar mayusculas; deja el nombre real y el servidor dueno de la ruta
    bool buscarCategoria(const std::string& categoria, std::string& nombreReal, ServidorRuta& servidor);

    // busca el producto sin importar mayusculas en todos los servidores activos, categoria por categoria.
    // si esta en varios, prefiere el primero que tenga stock para lo pedido; si ninguno alcanza, deja
    // el que tenga mas. si se indica un servidor, solo busca en ese
    bool buscarProducto(const std::string& producto, int pedido, const std::string& soloServidor,
                        ProductoIsla& encontrado);

    // productos de una categoria en un servidor (mensaje interno 20); false si no contesto
    bool productosDe(const ServidorRuta& servidor, const std::string& categoria, std::string& count,
                     std::string& lista);

    static std::string error(int codigo);
    static std::string noEncontrado(const std::string& valor);
};

// le devuelve a un servidor lo que se habia reservado y no se uso (mensaje interno 52).
// la usan AtencionIsla, cuando un servidor reservo menos de lo pedido, y el hilo que vence reservas
void devolverReserva(TablaRutas& rutas, const std::string& idServidor, const std::string& producto, int cantidad);

#endif
