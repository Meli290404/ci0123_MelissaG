#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"

class Bodega {
public:
    // idIntermediario e idTextoIntermediario son los del intermediario de la isla a la que pertenece
    Bodega( Buzon & buzon, int miId, const std::string & nombre,
             int idIntermediario, const std::string & idTextoIntermediario,
             std::vector<std::string> categorias, std::vector<Producto> productos );

    void ejecutar();

private:
    Buzon & buzon_;
    int miId_;
    std::string nombre_;
    int idInt_;
    std::string txtInt_;
    std::vector<std::string> categorias_;
    std::vector<Producto> productos_;

    void anunciarse();
    void atenderCategorias();
    void atenderProductos( const std::string & categoria );
    void atenderReserva( const std::string & nombre, const std::string & cantidadTexto );

    // envia la respuesta al intermediario y la rotula
    void responder( const std::string & msg, const std::string & explicacion );
    std::string listaCategorias() const;
};

#endif
