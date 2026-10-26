#include "Bodega.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

// ---------- formato de la linea ----------

static std::string centavosATexto(uint32_t centavos) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u.%02u", centavos / 100, centavos % 100);
    return buf;
}

// "1500.00" -> 150000. acepta tambien "1500" y "1500.5"
static bool textoACentavos(const std::string& texto, uint32_t& centavos) {
    size_t punto = texto.find('.');
    std::string entera = texto.substr(0, punto);
    std::string decimal = (punto == std::string::npos) ? "" : texto.substr(punto + 1);
    if (entera.empty() || entera.size() > 7 || decimal.size() > 2) return false;
    for (char c : entera + decimal) {
        if (c < '0' || c > '9') return false;
    }
    while (decimal.size() < 2) decimal += '0';
    centavos = (uint32_t)std::stoul(entera) * 100 + (uint32_t)std::stoul(decimal);
    return true;
}

std::string lineaATexto(const LineaProducto& p) {
    return p.bodega + "," + p.categoria + "," + p.nombre + "," + std::to_string(p.cantidad) + "," +
           centavosATexto(p.precioCentavos) + "\n";
}

bool textoALinea(const std::string& texto, LineaProducto& p) {
    std::vector<std::string> campos;
    std::stringstream ss(texto);
    std::string campo;
    while (std::getline(ss, campo, ',')) campos.push_back(campo);
    if (campos.size() != 5 || campos[3].empty() || campos[3].size() > 5) return false;

    for (char c : campos[3]) {
        if (c < '0' || c > '9') return false;
    }
    p.bodega = campos[0];
    p.categoria = campos[1];
    p.nombre = campos[2];
    p.cantidad = std::stoi(campos[3]);
    return textoACentavos(campos[4], p.precioCentavos);
}

// ---------- Bodega ----------

Bodega::Bodega(Filesystem& fs, uint16_t inodo, const std::string& nombreBodega)
    : fs_(fs), inodo_(inodo), nombreBodega_(nombreBodega) {}

std::vector<LineaProducto> Bodega::leerSinCandado() {
    std::vector<LineaProducto> lineas;
    std::stringstream ss(fs_.leerArchivo(inodo_));
    std::string texto;
    while (std::getline(ss, texto)) {
        LineaProducto p;
        if (textoALinea(texto, p)) lineas.push_back(p);
        // una linea danada se salta en lugar de tumbar toda la bodega
    }
    return lineas;
}

bool Bodega::guardarSinCandado(const std::vector<LineaProducto>& lineas) {
    std::string contenido;
    for (const auto& p : lineas) contenido += lineaATexto(p);
    return fs_.escribirArchivo(inodo_, contenido);
}

std::vector<LineaProducto> Bodega::leerLineas() {
    std::lock_guard<std::mutex> lock(fs_.mutex());
    return leerSinCandado();
}

std::vector<std::string> Bodega::listarCategorias() {
    std::vector<std::string> categorias;
    for (const auto& p : leerLineas()) {
        bool repetida = false;
        for (const auto& c : categorias) {
            if (c == p.categoria) repetida = true;
        }
        if (!repetida) categorias.push_back(p.categoria);
    }
    return categorias;
}

bool Bodega::tieneCategoria(const std::string& categoria) {
    for (const auto& p : leerLineas()) {
        if (p.categoria == categoria) return true;
    }
    return false;
}

std::vector<LineaProducto> Bodega::listarProductos(const std::string& categoria) {
    std::vector<LineaProducto> productos;
    for (const auto& p : leerLineas()) {
        if (p.categoria == categoria) productos.push_back(p);
    }
    return productos;
}

bool Bodega::agregarProducto(const std::string& categoria, const std::string& nombreProducto,
                             int cantidad, uint32_t precioCentavos) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    LineaProducto nuevo{nombreBodega_, categoria, nombreProducto, cantidad, precioCentavos};

    // agregar al final no mueve las lineas anteriores, asi que basta con pegar el texto nuevo
    std::string contenido = fs_.leerArchivo(inodo_) + lineaATexto(nuevo);
    return fs_.escribirArchivo(inodo_, contenido);
}

ResultadoStock Bodega::descontarStock(const std::string& nombre, int cantidad, int& stockActual) {
    // un solo candado para que ningun otro hilo cambie el stock entre la revision y la escritura
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<LineaProducto> lineas = leerSinCandado();
    for (auto& p : lineas) {
        if (p.nombre != nombre) continue;

        stockActual = p.cantidad;
        if (stockActual < cantidad) return STOCK_INSUFICIENTE;

        p.cantidad -= cantidad;
        stockActual = p.cantidad;
        // la cantidad puede cambiar de largo ("10" -> "9"), por eso se reescribe el archivo completo
        guardarSinCandado(lineas);
        return STOCK_DESCONTADO;
    }
    return PRODUCTO_NO_EXISTE;
}

bool Bodega::reservarStock(const std::string& nombre, int pedido, int& reservado, uint32_t& precioCentavos) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<LineaProducto> lineas = leerSinCandado();
    for (auto& p : lineas) {
        if (p.nombre != nombre) continue;

        // si no alcanza se reserva lo que haya disponible
        reservado = (p.cantidad < pedido) ? p.cantidad : pedido;
        precioCentavos = p.precioCentavos;

        if (reservado > 0) {
            p.cantidad -= reservado;
            guardarSinCandado(lineas);
        }
        return true;
    }
    return false;
}
