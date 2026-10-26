#ifndef ADMINISTRADORBODEGAS_H
#define ADMINISTRADORBODEGAS_H

#include <string>
#include <vector>
#include <cstdint>
#include "FileSystem.h"
#include "Inventario.h"
#include "Bodega.h"

// unico punto para cambiar bodegas, categorias y productos dentro del archivo.
// valida antes de escribir para que nada se recorte en silencio con strncpy
class AdministradorBodegas {
public:
    AdministradorBodegas(Filesystem& fs);

    // la bodega se identifica solo por su nombre, que no se repite
    bool agregarBodega(const std::string& nombre, std::string& error);
    bool eliminarBodega(const std::string& nombre, std::string& error);

    // la categoria no se crea aparte: existe en cuanto una linea de la bodega la menciona
    bool agregarProducto(const std::string& bodega, const std::string& categoria,
                         const std::string& nombre, int cantidad, uint32_t precioCentavos,
                         std::string& error);

    // descuenta del producto en la bodega donde este; stockActual queda como en Bodega::descontarStock
    bool descontarStock(const std::string& nombre, int cantidad, int& stockActual, std::string& error);

    // recorre todo el archivo y arma las vistas que usa PaginaWeb
    std::vector<BodegaVista> leerInventario();

    // todas las categorias de todas las bodegas
    std::vector<std::string> listarCategorias();

    // productos de la categoria en la bodega donde este; devuelve false si ninguna la tiene
    bool productosDeCategoria(const std::string& categoria, std::vector<LineaProducto>& productos);

    // reserva en la bodega donde este el producto; devuelve false si ninguna lo tiene
    bool reservar(const std::string& nombre, int pedido, int& reservado, uint32_t& precioCentavos);

private:
    Filesystem& fs_;

    // devuelve el nombre de la bodega que ya tiene la categoria, o "" si ninguna
    std::string bodegaConCategoria(const std::string& categoria);
    bool productoExiste(const std::string& nombre);
};

#endif