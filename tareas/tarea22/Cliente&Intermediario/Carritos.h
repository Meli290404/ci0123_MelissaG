#ifndef CARRITOS_H
#define CARRITOS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>

// un producto ya reservado, con el precio que se confirmo. si viene de otra isla, isla trae su
// INT_NN y la reserva vive alla (50 del mancomunado); si no, isla va vacio
struct ItemReservado {
    std::string producto;
    int cantidad;
    uint32_t precioCentavos;
    std::string isla;
};

// guarda el carrito de cada sesion en la memoria del intermediario.
// varios hilos atienden clientes al mismo tiempo, por eso todo pasa por mutex_
class Carritos {
public:
    // reparte un numero de sesion nuevo con su carrito vacio
    int nuevaSesion();
    bool existe(int sesion);

    // si el producto ya estaba con el mismo precio y de la misma isla se suman las cantidades
    bool agregar(int sesion, const ItemReservado& item);

    // entrega los productos del carrito y lo deja vacio
    bool vaciar(int sesion, std::vector<ItemReservado>& items);

private:
    std::mutex mutex_;
    std::map<int, std::vector<ItemReservado>> carritos_;
    int siguienteSesion_ = 1;
};

#endif