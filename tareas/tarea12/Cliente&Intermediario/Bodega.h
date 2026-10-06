#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>
#include <cstdint>
#include "Formatos.h"
#include "FileSystem.h"

// resultado de descontarStock
enum ResultadoStock { STOCK_DESCONTADO, STOCK_INSUFICIENTE, PRODUCTO_NO_EXISTE };

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

    // devuelve al bitmap todos los bloques de esta bodega: directorio de categorias, indices y datos
    void liberarContenido();

    // busca el producto en esta bodega y, si alcanza, resta la cantidad y escribe el bloque.
    // stockActual devuelve el stock nuevo si se desconto, o el disponible si no alcanzo
    ResultadoStock descontarStock(const std::string& nombre, int cantidad, int& stockActual);

    // reserva hasta la cantidad pedida; si no alcanza reserva lo que haya.
    // devuelve false si el producto no esta en esta bodega
    bool reservarStock(const std::string& nombre, int pedido, int& reservado, uint32_t& precioCentavos);

private:
    Filesystem& fs_;
    uint16_t bloqueDirCategorias_;

    bool ubicarCasillaCategoria(const std::string& id, int& bloqueOut, int& posOut);

    // encuentra el bloque de datos y la posicion del producto; quien la llama ya debe tener el mutex
    bool ubicarProducto(const std::string& nombre, int& bloqueOut, int& posOut);
};

#endif