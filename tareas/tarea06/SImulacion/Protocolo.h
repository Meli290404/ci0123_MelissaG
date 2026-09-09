#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>

// Identificador numérico de cada componente. Se usan para armar el "canal"
// de cada mensaje 
const int ID_CLIENTE       = 1;
const int ID_INTERMEDIARIO = 2;
const int ID_BODEGA        = 3;


// El mtype identifica el CANAL, quién habla con quién, no solo
// el destinatario. Se calcula como origen*10 + destino. Así:
//   Cliente -> Intermediario  = 12
//   Intermediario -> Cliente  = 21
//   Intermediario -> Bodega   = 23
//   Bodega -> Intermediario   = 32
// y cada componente puede pedir exactamente el canal que le interesa en cada
// momento.
inline long canal( int origen, int destino ) {
    return origen * 10 + destino;
}

// Canal que usa la bodega para anunciarse al arrancar (mensaje
// HOLA)incluye el ID de la bodega como primer dato.
const long CANAL_REGISTRO = 99;

// Tamaño máximo de mensaje 
const int TAM_MAX_MENSAJE = 256;

// Un producto de bodega
struct Producto {
    std::string nombre;
    float precio;
    int stock;
    std::string descripcion;
};

// Item ya confirmado en el carrito via ADD_CART: precio y stock
// ya fueron validados contra la bodega en el momento de agregarlo.
struct ItemCarrito {
    std::string nombre;
    float precio;
    int cantidad;
};

#endif