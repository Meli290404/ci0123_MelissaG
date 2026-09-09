#ifndef INTERMEDIARIO_H
#define INTERMEDIARIO_H

#include <set>
#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"

// Representa el "servidor intermediario". No tiene datos propios de productos.
// Comunica la bodega con el cliente
class Intermediario {
public:
    explicit Intermediario( Buzon & buzon );

    void ejecutar();

private:
    Buzon & buzon_;

    int idBodega_;                               // id de la bodega, extraido del HOLA
    std::string nombreBodega_;                   // "Bodega", para los rótulos y ERR_COMM
    std::set<std::string> categoriasAtendidas_;  // categorías que sabemos que existen
    std::vector<ItemCarrito> carrito_;           // productos ya confirmados con ADD_CART

    void registrarBodega();
    void manejarReCat( const std::string & categoria );
    void manejarReProd( const std::string & nombre );
    void manejarAddCart( const std::string & producto, const std::string & countTexto );
    void manejarGetFact( const std::string & countTexto );
};

#endif