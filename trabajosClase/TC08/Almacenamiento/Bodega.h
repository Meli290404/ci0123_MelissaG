#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>
#include <cstdint>
#include "Formatos.h"
#include "FileSystem.h"

// representa una bodega dentro de cafeteria.dat. no tiene archivo propio:
// todas las bodegas comparten el mismo Filesystem y el mismo bitmap.
class Bodega {
public:
    // bloqueDirCategorias viene de Filesystem::registrarBodega o buscarBodega
    Bodega(Filesystem& fs, uint16_t bloqueDirCategorias);

    void crearCategoria(const std::string& nombre, const std::string& id);
    bool buscarCategoria(const std::string& id, DirCategoria& resultado);
    std::vector<DirCategoria> listarCategorias();

    bool agregarProducto(const std::string& idCategoria, const std::string& nombreProducto, int cantidad, uint32_t precioCentavos);
    std::vector<RegistroProducto> listarProductos(const std::string& idCategoria);

    // insertarStock busca el producto por nombre dentro de la categoria: si ya existe,
    // le suma la cantidad al stock que tenia y actualiza el precio; si no existe,
    // crea un registro nuevo (mismo mecanismo de crecimiento que agregarProducto)
    bool insertarStock(const std::string& idCategoria, const std::string& nombreProducto,
                        int cantidad, uint32_t precioCentavos);

    // extraerStock busca el producto por nombre y le resta la cantidad pedida,
    // solo si hay suficiente stock disponible
    bool extraerStock(const std::string& idCategoria, const std::string& nombreProducto, int cantidad);

private:
    Filesystem& fs_;
    uint16_t bloqueDirCategorias_;

    bool ubicarCasillaCategoria(const std::string& id, int& bloqueOut, int& posOut);
};

#endif