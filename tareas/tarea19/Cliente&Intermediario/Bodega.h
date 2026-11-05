#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>
#include <cstdint>
#include "Formatos.h"
#include "FileSystem.h"

// resultado de descontarStock
enum ResultadoStock { STOCK_DESCONTADO, STOCK_INSUFICIENTE, PRODUCTO_NO_EXISTE };

// una linea del archivo de texto de la bodega:
//   nombreBodega,categoria,producto,cantidad,precio
// la categoria es el mismo valor que viaja en el protocolo (^[a-zA-Z]{3,20}$)
struct LineaProducto {
    std::string bodega;
    std::string categoria;
    std::string nombre;
    int cantidad;
    uint32_t precioCentavos;
};

// representa una bodega dentro de fs.dat.ci0123. es un archivo de texto con su propio i-nodo;
// todas las bodegas comparten el mismo Filesystem y el mismo bitmap
class Bodega {
public:
    // inodo viene de Filesystem::registrarBodega o de la entrada del directorio
    Bodega(Filesystem& fs, uint16_t inodo, const std::string& nombreBodega);

    // todas las lineas de la bodega, en el orden en que estan en el archivo
    std::vector<LineaProducto> leerLineas();

    // categorias sin repetir, en orden de aparicion
    std::vector<std::string> listarCategorias();
    bool tieneCategoria(const std::string& categoria);

    std::vector<LineaProducto> listarProductos(const std::string& categoria);

    // agrega una linea al final del archivo; false si no hay bloques o el archivo llego al maximo
    bool agregarProducto(const std::string& categoria, const std::string& nombreProducto,
                         int cantidad, uint32_t precioCentavos);

    // busca el producto en esta bodega y, si alcanza, resta la cantidad y reescribe el archivo.
    // stockActual devuelve el stock nuevo si se desconto, o el disponible si no alcanzo
    ResultadoStock descontarStock(const std::string& nombre, int cantidad, int& stockActual);

    // reserva hasta la cantidad pedida; si no alcanza reserva lo que haya.
    // devuelve false si el producto no esta en esta bodega
    bool reservarStock(const std::string& nombre, int pedido, int& reservado, uint32_t& precioCentavos);

private:
    Filesystem& fs_;
    uint16_t inodo_;
    std::string nombreBodega_;

    // quien las llama ya debe tener el mutex
    std::vector<LineaProducto> leerSinCandado();
    bool guardarSinCandado(const std::vector<LineaProducto>& lineas);
};

// conversiones entre la linea de texto y LineaProducto
std::string lineaATexto(const LineaProducto& p);
bool textoALinea(const std::string& texto, LineaProducto& p);

#endif
