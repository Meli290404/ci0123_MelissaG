#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"

// Representa el "servidor de productos". No sabe nada de clientes, solo recibe mensajes del intermediario 
//y responde con la información de productos que tiene en su inventario.
class Bodega {
public:
    Bodega( Buzon & buzon, int miId, const std::string & nombre,
            std::vector<std::string> categorias, std::vector<Producto> productos );

    // Ciclo principal del hilo: se anuncia una vez y luego atiende mensajes hasta recibir EXIT.
    void ejecutar();

private:
    Buzon & buzon_;  
    int miId_;  
    std::string nombre_;
    std::vector<std::string> categorias_;
    std::vector<Producto> productos_;

    void anunciarse();
    void atenderListProd( const std::string & categoria );
    void atenderFindProd( const std::string & nombre );
};

#endif