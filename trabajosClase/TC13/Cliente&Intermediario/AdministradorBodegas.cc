#include "AdministradorBodegas.h"
#include "Bodega.h"
#include "Validaciones.h"
#include <cstring>
#include <cstdio>
#include <cctype>

static std::string textoCampo(const char* campo, size_t tam) {
    return std::string(campo, strnlen(campo, tam));
}

// el texto cabe si no esta vacio y deja espacio para el '\0' que pone strncpy
static bool cabeEnCampo(const std::string& texto, size_t tamCampo) {
    return !texto.empty() && texto.size() <= tamCampo - 1;
}

static bool esAlfanumerico(const std::string& texto) {
    for (char c : texto) {
        if (!isalnum((unsigned char)c)) return false;
    }
    return true;
}

// los separadores del protocolo romperian los mensajes si aparecen dentro de un nombre
static bool sinSeparadores(const std::string& texto) {
    return texto.find_first_of("/,;|") == std::string::npos;
}

static std::string precioTexto(uint32_t centavos) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%02u", centavos / 100, centavos % 100);
    return buf;
}

AdministradorBodegas::AdministradorBodegas(Filesystem& fs) : fs_(fs) {}

bool AdministradorBodegas::agregarBodega(const std::string& id, const std::string& nombre, std::string& error) {
    if (!cabeEnCampo(id, sizeof(DirBodega::id)) || !esAlfanumerico(id)) {
        error = "el id de bodega debe tener entre 1 y " + std::to_string(sizeof(DirBodega::id) - 1) +
                " letras o digitos";
        return false;
    }
    if (!cabeEnCampo(nombre, sizeof(DirBodega::nombre)) || !sinSeparadores(nombre)) {
        error = "el nombre de bodega debe tener entre 1 y " + std::to_string(sizeof(DirBodega::nombre) - 1) +
                " caracteres, sin / , ; |";
        return false;
    }

    DirBodega existente;
    if (fs_.buscarBodega(id, existente)) {
        error = "ya existe una bodega con id " + id;
        return false;
    }

    if (fs_.registrarBodega(nombre, id) == BLOQUE_NULO) {
        error = "no hay bloques libres para registrar la bodega";
        return false;
    }
    return true;
}

bool AdministradorBodegas::eliminarBodega(const std::string& id, std::string& error) {
    // primero se quita del directorio, asi ninguna busqueda la encuentra mientras se liberan sus bloques
    DirBodega quitada;
    if (!fs_.quitarBodega(id, quitada)) {
        error = "no existe una bodega con id " + id;
        return false;
    }

    Bodega bodega(fs_, quitada.bloqueDirCategorias);
    bodega.liberarContenido();
    return true;
}

bool AdministradorBodegas::agregarCategoria(const std::string& idBodega, const std::string& idCategoria,
                                            const std::string& nombre, std::string& error) {
    // el id debe cumplir validarCategoria (al menos 3 letras) y caber en DirCategoria::id
    if (!cabeEnCampo(idCategoria, sizeof(DirCategoria::id)) || !validarCategoria(idCategoria)) {
        error = "el id de categoria debe tener entre 3 y " + std::to_string(sizeof(DirCategoria::id) - 1) +
                " letras";
        return false;
    }
    if (!cabeEnCampo(nombre, sizeof(DirCategoria::nombre)) || !sinSeparadores(nombre)) {
        error = "el nombre de categoria debe tener entre 1 y " + std::to_string(sizeof(DirCategoria::nombre) - 1) +
                " caracteres, sin / , ; |";
        return false;
    }

    DirBodega dirBod;
    if (!fs_.buscarBodega(idBodega, dirBod)) {
        error = "no existe una bodega con id " + idBodega;
        return false;
    }

    // se revisa en todas las bodegas porque las categorias se buscan solo por id
    if (categoriaExiste(idCategoria)) {
        error = "ya existe una categoria con id " + idCategoria;
        return false;
    }

    Bodega bodega(fs_, dirBod.bloqueDirCategorias);
    bodega.crearCategoria(nombre, idCategoria);

    // crearCategoria no devuelve si fallo, asi que se confirma buscandola
    DirCategoria creada;
    if (!bodega.buscarCategoria(idCategoria, creada)) {
        error = "no hay bloques libres para crear la categoria";
        return false;
    }
    return true;
}

bool AdministradorBodegas::agregarProducto(const std::string& idBodega, const std::string& idCategoria,
                                           const std::string& nombre, int cantidad, uint32_t precioCentavos,
                                           std::string& error) {
    if (!validarProducto(nombre) || !cabeEnCampo(nombre, sizeof(RegistroProducto::nombre))) {
        error = "el nombre del producto solo puede tener letras, digitos o guion, hasta " +
                std::to_string(sizeof(RegistroProducto::nombre) - 1) + " caracteres";
        return false;
    }
    if (cantidad < 0 || cantidad > UINT16_MAX) {
        error = "la cantidad debe estar entre 0 y " + std::to_string(UINT16_MAX);
        return false;
    }

    DirBodega dirBod;
    if (!fs_.buscarBodega(idBodega, dirBod)) {
        error = "no existe una bodega con id " + idBodega;
        return false;
    }

    Bodega bodega(fs_, dirBod.bloqueDirCategorias);
    DirCategoria cat;
    if (!bodega.buscarCategoria(idCategoria, cat)) {
        error = "la bodega " + idBodega + " no tiene la categoria " + idCategoria;
        return false;
    }

    // los productos se buscan solo por nombre, por eso no se permiten repetidos
    if (productoExiste(nombre)) {
        error = "ya existe un producto llamado " + nombre;
        return false;
    }

    // con los MAX_BLOQUES_INDICE bloques llenos ya no hay donde anotar otro bloque de datos
    size_t capacidad = (size_t)MAX_BLOQUES_INDICE * PRODUCTOS_POR_BLOQUE;
    if (bodega.listarProductos(idCategoria).size() >= capacidad) {
        error = "la categoria llego al maximo de " + std::to_string(capacidad) + " productos";
        return false;
    }

    if (!bodega.agregarProducto(idCategoria, nombre, cantidad, precioCentavos)) {
        error = "no hay bloques libres para el producto";
        return false;
    }
    return true;
}

bool AdministradorBodegas::descontarStock(const std::string& nombre, int cantidad, int& stockActual,
                                          std::string& error) {
    if (cantidad <= 0) {
        error = "la cantidad a descontar debe ser mayor que 0";
        return false;
    }

    // los nombres de producto no se repiten entre bodegas, asi que solo una bodega lo puede tener
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        ResultadoStock resultado = bodega.descontarStock(nombre, cantidad, stockActual);

        if (resultado == STOCK_DESCONTADO) return true;
        if (resultado == STOCK_INSUFICIENTE) {
            error = "stock insuficiente de " + nombre + ": hay " + std::to_string(stockActual);
            return false;
        }
        // PRODUCTO_NO_EXISTE en esta bodega: se sigue con la siguiente
    }

    error = "no existe un producto llamado " + nombre;
    return false;
}

std::vector<BodegaVista> AdministradorBodegas::leerInventario() {
    std::vector<BodegaVista> inventario;

    for (auto& dirBod : fs_.listarBodegas()) {
        BodegaVista bodegaVista;
        bodegaVista.id = textoCampo(dirBod.id, sizeof(dirBod.id));
        bodegaVista.nombre = textoCampo(dirBod.nombre, sizeof(dirBod.nombre));

        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        for (auto& cat : bodega.listarCategorias()) {
            CategoriaVista categoriaVista;
            categoriaVista.id = textoCampo(cat.id, sizeof(cat.id));
            categoriaVista.nombre = textoCampo(cat.nombre, sizeof(cat.nombre));

            for (auto& p : bodega.listarProductos(categoriaVista.id)) {
                categoriaVista.productos.push_back({
                    textoCampo(p.nombre, sizeof(p.nombre)),
                    precioTexto(p.precioCentavos),
                    std::to_string(p.cantidad)
                });
            }
            bodegaVista.categorias.push_back(categoriaVista);
        }
        inventario.push_back(bodegaVista);
    }
    return inventario;
}

std::vector<std::string> AdministradorBodegas::listarIdsCategorias() {
    std::vector<std::string> ids;
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        for (auto& cat : bodega.listarCategorias()) {
            ids.push_back(textoCampo(cat.id, sizeof(cat.id)));
        }
    }
    return ids;
}

bool AdministradorBodegas::productosDeCategoria(const std::string& idCategoria,
                                                std::vector<RegistroProducto>& productos) {
    productos.clear();
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        DirCategoria cat;
        if (bodega.buscarCategoria(idCategoria, cat)) {
            productos = bodega.listarProductos(idCategoria);
            return true;
        }
    }
    return false;
}

bool AdministradorBodegas::reservar(const std::string& nombre, int pedido, int& reservado,
                                    uint32_t& precioCentavos) {
    // los nombres de producto no se repiten entre bodegas, asi que solo una bodega lo puede tener
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        if (bodega.reservarStock(nombre, pedido, reservado, precioCentavos)) return true;
    }
    return false;
}

bool AdministradorBodegas::categoriaExiste(const std::string& idCategoria) {
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        DirCategoria cat;
        if (bodega.buscarCategoria(idCategoria, cat)) return true;
    }
    return false;
}

bool AdministradorBodegas::productoExiste(const std::string& nombre) {
    for (auto& dirBod : fs_.listarBodegas()) {
        Bodega bodega(fs_, dirBod.bloqueDirCategorias);
        for (auto& cat : bodega.listarCategorias()) {
            for (auto& p : bodega.listarProductos(textoCampo(cat.id, sizeof(cat.id)))) {
                if (textoCampo(p.nombre, sizeof(p.nombre)) == nombre) return true;
            }
        }
    }
    return false;
}