#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"

class Bodega {
public:
    Bodega( Buzon & buzon, int miId, const std::string & nombre,
             std::vector<std::string> categorias, std::vector<Producto> productos );

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