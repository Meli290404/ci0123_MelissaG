#ifndef PAGINAWEB_H
#define PAGINAWEB_H

#include <string>
#include <vector>
#include "Inventario.h"

// arma el html de las paginas. no sabe de donde vienen los datos: recibe las vistas ya llenas
class PaginaWeb {
public:
    std::string inventario(const std::vector<BodegaVista>& bodegas);
    std::string error(int codigo, const std::string& mensaje);

private:
    std::string seccionBodega(const BodegaVista& bodega);
    std::string tablaProductos(const CategoriaVista& categoria);
    std::string envolver(const std::string& titulo, const std::string& contenido);
    std::string escapar(const std::string& texto);
};

#endif