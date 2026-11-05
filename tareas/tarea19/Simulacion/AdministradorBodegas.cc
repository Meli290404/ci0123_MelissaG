#include "AdministradorBodegas.h"
#include "Validaciones.h"
#include <cstring>
#include <cstdio>
#include <cctype>

static const int MAX_CANTIDAD = 99999;   // validarStock acepta hasta 5 digitos

static std::string textoCampo(const char* campo, size_t tam) {
    return std::string(campo, strnlen(campo, tam));
}

// el nombre de la bodega se escribe en los comandos y va al inicio de cada linea del archivo,
// por eso solo se permiten letras, digitos y guion (sin espacios ni comas)
static bool nombreBodegaValido(const std::string& nombre) {
    if (nombre.empty() || nombre.size() > sizeof(EntradaDirectorio::nombre) - 1) return false;
    for (char c : nombre) {
        if (!isalnum((unsigned char)c) && c != '-') return false;
    }
    return true;
}

static std::string precioTexto(uint32_t centavos) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%02u", centavos / 100, centavos % 100);
    return buf;
}

static BodegaFS abrirBodega(Filesystem& fs, const EntradaDirectorio& entrada) {
    return BodegaFS(fs, entrada.inodo, textoCampo(entrada.nombre, sizeof(entrada.nombre)));
}

AdministradorBodegas::AdministradorBodegas(Filesystem& fs) : fs_(fs) {}

bool AdministradorBodegas::agregarBodega(const std::string& nombre, std::string& error) {
    if (!nombreBodegaValido(nombre)) {
        error = "el nombre de bodega debe tener entre 1 y " + std::to_string(sizeof(EntradaDirectorio::nombre) - 1) +
                " letras, digitos o guion";
        return false;
    }

    EntradaDirectorio existente;
    if (fs_.buscarBodega(nombre, existente)) {
        error = "ya existe una bodega llamada " + nombre;
        return false;
    }

    if (fs_.registrarBodega(nombre) == BLOQUE_NULO) {
        error = "no hay bloques libres para registrar la bodega";
        return false;
    }
    return true;
}

bool AdministradorBodegas::eliminarBodega(const std::string& nombre, std::string& error) {
    // Filesystem la quita del directorio y libera su i-nodo con todos sus bloques de datos
    if (!fs_.eliminarBodega(nombre)) {
        error = "no existe una bodega llamada " + nombre;
        return false;
    }
    return true;
}

bool AdministradorBodegas::agregarProducto(const std::string& bodega, const std::string& categoria,
                                           const std::string& nombre, int cantidad, uint32_t precioCentavos,
                                           std::string& error) {
    // la categoria es la misma que viaja en el protocolo, asi que debe cumplir su regex
    if (!validarCategoria(categoria)) {
        error = "la categoria debe tener entre 3 y 20 letras, sin espacios";
        return false;
    }
    if (!validarProducto(nombre)) {
        error = "el nombre del producto solo puede tener letras, digitos o guion, hasta 50 caracteres";
        return false;
    }
    if (cantidad < 0 || cantidad > MAX_CANTIDAD) {
        error = "la cantidad debe estar entre 0 y " + std::to_string(MAX_CANTIDAD);
        return false;
    }

    EntradaDirectorio entrada;
    if (!fs_.buscarBodega(bodega, entrada)) {
        error = "no existe una bodega llamada " + bodega;
        return false;
    }

    // productosDeCategoria devuelve la primera bodega que tenga la categoria,
    // asi que una categoria vive en una sola bodega
    std::string duena = bodegaConCategoria(categoria);
    if (!duena.empty() && duena != bodega) {
        error = "la categoria " + categoria + " ya esta en la bodega " + duena;
        return false;
    }

    // los productos se buscan solo por nombre, por eso no se permiten repetidos
    if (productoExiste(nombre)) {
        error = "ya existe un producto llamado " + nombre;
        return false;
    }

    if (!abrirBodega(fs_, entrada).agregarProducto(categoria, nombre, cantidad, precioCentavos)) {
        error = "no hay bloques libres o la bodega llego a su tamano maximo (" +
                std::to_string(MAX_TAMANO_ARCHIVO) + " bytes)";
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
    for (auto& entrada : fs_.listarBodegas()) {
        BodegaFS bodega = abrirBodega(fs_, entrada);
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

    for (auto& entrada : fs_.listarBodegas()) {
        BodegaVista bodegaVista;
        // la vista conserva id y nombre; ahora los dos son el nombre de la bodega
        bodegaVista.nombre = textoCampo(entrada.nombre, sizeof(entrada.nombre));
        bodegaVista.id = bodegaVista.nombre;

        // se lee el archivo una sola vez y se agrupan las lineas por categoria
        BodegaFS bodega = abrirBodega(fs_, entrada);
        for (const auto& p : bodega.leerLineas()) {
            CategoriaVista* categoria = nullptr;
            for (auto& c : bodegaVista.categorias) {
                if (c.id == p.categoria) categoria = &c;
            }
            if (categoria == nullptr) {
                // la vista conserva id y nombre para PaginaWeb; ahora los dos son la misma palabra
                bodegaVista.categorias.push_back({p.categoria, p.categoria, {}});
                categoria = &bodegaVista.categorias.back();
            }
            categoria->productos.push_back({p.nombre, precioTexto(p.precioCentavos), std::to_string(p.cantidad)});
        }
        inventario.push_back(bodegaVista);
    }
    return inventario;
}

std::vector<std::string> AdministradorBodegas::listarCategorias() {
    std::vector<std::string> categorias;
    for (auto& entrada : fs_.listarBodegas()) {
        for (auto& c : abrirBodega(fs_, entrada).listarCategorias()) categorias.push_back(c);
    }
    return categorias;
}

bool AdministradorBodegas::productosDeCategoria(const std::string& categoria,
                                                std::vector<LineaProducto>& productos) {
    productos.clear();
    for (auto& entrada : fs_.listarBodegas()) {
        productos = abrirBodega(fs_, entrada).listarProductos(categoria);
        if (!productos.empty()) return true;
    }
    return false;
}

bool AdministradorBodegas::reservar(const std::string& nombre, int pedido, int& reservado,
                                    uint32_t& precioCentavos) {
    // los nombres de producto no se repiten entre bodegas, asi que solo una bodega lo puede tener
    for (auto& entrada : fs_.listarBodegas()) {
        if (abrirBodega(fs_, entrada).reservarStock(nombre, pedido, reservado, precioCentavos)) return true;
    }
    return false;
}

std::string AdministradorBodegas::bodegaConCategoria(const std::string& categoria) {
    for (auto& entrada : fs_.listarBodegas()) {
        if (abrirBodega(fs_, entrada).tieneCategoria(categoria)) {
            return textoCampo(entrada.nombre, sizeof(entrada.nombre));
        }
    }
    return "";
}

bool AdministradorBodegas::productoExiste(const std::string& nombre) {
    for (auto& entrada : fs_.listarBodegas()) {
        for (const auto& p : abrirBodega(fs_, entrada).leerLineas()) {
            if (p.nombre == nombre) return true;
        }
    }
    return false;
}
