#include "Despachador.h"
#include "Validaciones.h"
#include <cstring>
#include <cstdio>
#include <sstream>

std::string formatearPrecio(uint32_t centavos) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", centavos / 100.0);
    return std::string(buf);
}

bool validarMensaje(const Mensaje& msg, std::string& campoInvalido, std::string& valorInvalido) {
    if (!validarOrigenDestino(msg.origen)) { campoInvalido = "origen"; valorInvalido = msg.origen; return false; }
    if (!validarOrigenDestino(msg.destino)) { campoInvalido = "destino"; valorInvalido = msg.destino; return false; }

    switch (msg.tipo) {
        case TIPO_REQUEST_CATEGORY:
            break; // categoria opcional, vacio = listar todas
        case TIPO_REQUEST_PRODUCTS:
        case TIPO_REQUEST_DETAIL:
            if (msg.campos.empty()) { campoInvalido = "campo1"; valorInvalido = ""; return false; }
            break;
        case TIPO_ADD_TO_CART:
            if (msg.campos.size() < 2 || !validarProducto(msg.campos[0]) || !validarCount(msg.campos[1])) {
                campoInvalido = "producto/count";
                valorInvalido = msg.campos.empty() ? "" : msg.campos[0];
                return false;
            }
            break;
        case TIPO_REQUEST_FACTURA:
            if (!msg.campos.empty() && !validarCount(msg.campos[0])) {
                campoInvalido = "count";
                valorInvalido = msg.campos[0];
                return false;
            }
            break;
        default:
            break;
    }
    return true;
}

Despachador::Despachador(Bodega& bodega1, Bodega& bodega2) : bodega1_(bodega1), bodega2_(bodega2) {}

Bodega* Despachador::encontrarBodegaConCategoria(const std::string& idCategoria, DirCategoria& catOut) {
    if (bodega1_.buscarCategoria(idCategoria, catOut)) return &bodega1_;
    if (bodega2_.buscarCategoria(idCategoria, catOut)) return &bodega2_;
    return nullptr;
}

bool Despachador::buscarProductoPorNombre(const std::string& nombre, RegistroProducto& prodOut) {
    for (Bodega* bod : { &bodega1_, &bodega2_ }) {
        for (auto& cat : bod->listarCategorias()) {
            std::string idCat(cat.id, strnlen(cat.id, sizeof(cat.id)));
            for (auto& p : bod->listarProductos(idCat)) {
                std::string nombreProd(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
                if (nombreProd == nombre) { prodOut = p; return true; }
            }
        }
    }
    return false;
}

void Despachador::manejarRequestCategory(int& tipoResp, std::string& html) {
    std::ostringstream oss;
    oss << "<div class=\"respuesta\" data-tipo=\"" << TIPO_CATEGORY_LIST << "\">\n<ul class=\"categorias\">\n";
    for (Bodega* bod : { &bodega1_, &bodega2_ }) {
        for (auto& cat : bod->listarCategorias()) {
            std::string id(cat.id, strnlen(cat.id, sizeof(cat.id)));
            std::string nombre(cat.nombre, strnlen(cat.nombre, sizeof(cat.nombre)));
            oss << "<li data-id=\"" << id << "\">" << nombre << "</li>\n";
        }
    }
    oss << "</ul>\n</div>";
    html = oss.str();
    tipoResp = TIPO_CATEGORY_LIST;
}

void Despachador::manejarRequestProducts(const std::string& idCategoria, int& tipoResp, std::string& html) {
    DirCategoria cat;
    Bodega* bod = encontrarBodegaConCategoria(idCategoria, cat);
    std::vector<RegistroProducto> productos = (bod != nullptr) ? bod->listarProductos(idCategoria) : std::vector<RegistroProducto>{};

    if (bod == nullptr || productos.empty()) {
        html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_NO_PRODUCTS) +
               "\"><p>No hay productos disponibles en la categoria \"" + idCategoria + "\"</p></div>";
        tipoResp = TIPO_NO_PRODUCTS;
        return;
    }

    std::ostringstream oss;
    oss << "<div class=\"respuesta\" data-tipo=\"" << TIPO_PRODUCT_LIST << "\">\n<table class=\"productos\">\n";
    for (auto& p : productos) {
        std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
        std::string precio = formatearPrecio(p.precioCentavos);
        oss << "<tr data-nombre=\"" << nombre << "\" data-precio=\"" << precio
            << "\" data-stock=\"" << p.cantidad << "\">"
            << "<td>" << nombre << "</td><td>" << precio << "</td><td>" << p.cantidad << "</td></tr>\n";
    }
    oss << "</table>\n</div>";
    html = oss.str();
    tipoResp = TIPO_PRODUCT_LIST;
}

void Despachador::manejarRequestDetail(const std::string& nombreProducto, int& tipoResp, std::string& html) {
    RegistroProducto p;
    if (!buscarProductoPorNombre(nombreProducto, p)) {
        html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_PRODUCT_NOT_FOUND) +
               "\"><p>\"" + nombreProducto + "\" no encontrado</p></div>";
        tipoResp = TIPO_PRODUCT_NOT_FOUND;
        return;
    }
    std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
    std::string precio = formatearPrecio(p.precioCentavos);
    html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_PRODUCT_DETAIL) +
           "\" data-nombre=\"" + nombre + "\" data-precio=\"" + precio +
           "\" data-stock=\"" + std::to_string(p.cantidad) + "\"><p>" + nombre + " - " + precio +
           " - stock " + std::to_string(p.cantidad) + "</p></div>";
    tipoResp = TIPO_PRODUCT_DETAIL;
}

void Despachador::manejarAddToCart(const std::string& producto, const std::string& countStr, int& tipoResp, std::string& html) {
    int cantidad = std::stoi(countStr);

    RegistroProducto p;
    if (!buscarProductoPorNombre(producto, p)) {
        html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_PRODUCT_NOT_FOUND) +
               "\"><p>\"" + producto + "\" no encontrado</p></div>";
        tipoResp = TIPO_PRODUCT_NOT_FOUND;
        return;
    }

    // solo se valida disponibilidad, no se descuenta stock en este paso
    if (p.cantidad < cantidad) {
        html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_NO_STOCK) +
               "\" data-producto=\"" + producto + "\" data-stock=\"" + std::to_string(p.cantidad) +
               "\" data-solicitado=\"" + countStr + "\"><p>Stock insuficiente</p></div>";
        tipoResp = TIPO_NO_STOCK;
        return;
    }

    std::string precio = formatearPrecio(p.precioCentavos);
    html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_CART_ADDED) +
           "\" data-producto=\"" + producto + "\" data-precio=\"" + precio +
           "\" data-count=\"" + countStr + "\"><p>Agregado " + countStr + " x " + producto +
           " @ " + precio + "</p></div>";
    tipoResp = TIPO_CART_ADDED;
}

void Despachador::manejarFactura(const std::vector<std::string>& campos, int& tipoResp, std::string& html) {
    int count = campos.empty() ? 0 : std::stoi(campos[0]);

    if (count == 0) {
        html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_CARRITO_VACIO) +
               "\"><p>El carrito esta vacio</p></div>";
        tipoResp = TIPO_CARRITO_VACIO;
        return;
    }

    std::string listaItems = campos.size() > 1 ? campos[1] : "";
    double total = 0.0;
    std::ostringstream filas;

    std::stringstream ssItems(listaItems);
    std::string item;
    while (std::getline(ssItems, item, ';')) {
        size_t coma = item.find(',');
        if (coma == std::string::npos) continue;
        std::string nombreProd = item.substr(0, coma);
        int cantidad = std::stoi(item.substr(coma + 1));

        RegistroProducto p;
        if (!buscarProductoPorNombre(nombreProd, p)) continue; // producto ya no existe, se omite

        double subtotal = (p.precioCentavos / 100.0) * cantidad;
        total += subtotal;

        char subtotalStr[16];
        snprintf(subtotalStr, sizeof(subtotalStr), "%.2f", subtotal);

        filas << "<tr data-producto=\"" << nombreProd << "\" data-cantidad=\"" << cantidad
              << "\" data-subtotal=\"" << subtotalStr << "\">"
              << "<td>" << nombreProd << "</td><td>" << cantidad << "</td><td>" << subtotalStr << "</td></tr>\n";
    }

    char totalStr[16];
    snprintf(totalStr, sizeof(totalStr), "%.2f", total);

    html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_FACTURA) + "\" data-total=\"" +
           totalStr + "\">\n<table class=\"factura\">\n" + filas.str() +
           "</table>\n<p class=\"total\">Total: " + totalStr + "</p>\n</div>";
    tipoResp = TIPO_FACTURA;
}

void Despachador::procesar(const Mensaje& msg, int& tipoResp, std::string& html) {
    switch (msg.tipo) {
        case TIPO_REQUEST_CATEGORY:
            manejarRequestCategory(tipoResp, html);
            break;
        case TIPO_REQUEST_PRODUCTS:
            manejarRequestProducts(msg.campos.empty() ? "" : msg.campos[0], tipoResp, html);
            break;
        case TIPO_REQUEST_DETAIL:
            manejarRequestDetail(msg.campos.empty() ? "" : msg.campos[0], tipoResp, html);
            break;
        case TIPO_ADD_TO_CART:
            manejarAddToCart(msg.campos[0], msg.campos[1], tipoResp, html);
            break;
        case TIPO_REQUEST_FACTURA:
            manejarFactura(msg.campos, tipoResp, html);
            break;
        default:
            html = "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_ERR_FORMATO) +
                   "\"><p>tipo de mensaje no soportado</p></div>";
            tipoResp = TIPO_ERR_FORMATO;
    }
}