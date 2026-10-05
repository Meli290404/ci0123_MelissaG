#ifndef ADMINISTRADORBODEGAS_H
#define ADMINISTRADORBODEGAS_H

#include <string>
#include <vector>
#include <cstdint>
#include "FileSystem.h"
#include "Inventario.h"

// unico punto para cambiar bodegas, categorias y productos dentro del archivo.
// valida antes de escribir para que nada se recorte en silencio con strncpy
class AdministradorBodegas {
public:
    AdministradorBodegas(Filesystem& fs);

    bool agregarBodega(const std::string& id, const std::string& nombre, std::string& error);
    bool eliminarBodega(const std::string& id, std::string& error);
    bool agregarCategoria(const std::string& idBodega, const std::string& idCategoria,
                          const std::string& nombre, std::string& error);
    bool agregarProducto(const std::string& idBodega, const std::string& idCategoria,
                         const std::string& nombre, int cantidad, uint32_t precioCentavos,
                         std::string& error);

    // recorre todo el archivo y arma las vistas que usa PaginaWeb
    std::vector<BodegaVista> leerInventario();

private:
    Filesystem& fs_;

    bool categoriaExiste(const std::string& idCategoria);
    bool productoExiste(const std::string& nombre);
};

#endif