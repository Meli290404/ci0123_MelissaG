#include "PaginaWeb.h"
#include <sstream>

static const char* ESTILOS =
    "body{font-family:sans-serif;margin:0;background:#f4f5f7;color:#222}"
    "header{background:#1f4e79;color:#fff;padding:12px 16px;font-weight:bold}"
    "main{max-width:760px;margin:0 auto;padding:16px}"
    "section{background:#fff;border-radius:6px;padding:4px 16px 16px;margin-bottom:16px}"
    "small{color:#666;font-weight:normal}"
    "table{width:100%;border-collapse:collapse}"
    "th,td{padding:6px;border-bottom:1px solid #ddd;text-align:left}"
    "nav{background:#163a5c;padding:8px 16px}"
    "nav a{color:#fff;margin-right:14px;text-decoration:none;font-size:14px}"
    "nav a:hover{text-decoration:underline}"
    ".menu{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px}"
    ".boton{display:block;background:#fff;border:1px solid #d0d7e2;border-radius:6px;padding:16px;"
    "text-decoration:none;color:#1f4e79}"
    ".boton:hover{border-color:#1f4e79}"
    ".boton b{display:block;font-size:18px;margin-bottom:4px}"
    ".boton span{color:#555;font-size:14px}"
    ".estado{color:#555;margin-bottom:16px}";

// paginas a las que se puede ir desde el menu y desde la barra de arriba
struct Opcion {
    const char* ruta;
    const char* nombre;
    const char* descripcion;
};
static const Opcion OPCIONES[] = {
    {"/inventario", "Productos", "Todos los productos de cada servidor de la isla y de las otras islas"},
    {"/categorias", "Categorías", "Lista de categorías; cada una lleva a sus productos"},
    {"/servidores", "Servidores", "Servidores de productos descubiertos y su estado"},
    {"/rutas", "Tabla de rutas", "Qué servidor atiende cada categoría"},
    {"/islas", "Otras islas", "Intermediarios de otras islas descubiertos con el mancomunado"},
};

std::string PaginaWeb::menu(int servidoresActivos, int islasActivas) {
    std::ostringstream oss;
    oss << "<p class=\"estado\">" << servidoresActivos
        << (servidoresActivos == 1 ? " servidor de productos activo, " : " servidores de productos activos, ")
        << islasActivas << (islasActivas == 1 ? " isla activa" : " islas activas") << "</p>\n<div class=\"menu\">\n";
    for (const auto& opcion : OPCIONES) {
        oss << "<a class=\"boton\" href=\"" << opcion.ruta << "\"><b>" << opcion.nombre << "</b><span>"
            << opcion.descripcion << "</span></a>\n";
    }
    oss << "</div>\n";
    return envolver("Inicio", oss.str());
}

std::string PaginaWeb::inventario(const std::vector<BodegaVista>& bodegas) {
    std::ostringstream oss;
    if (bodegas.empty()) {
        oss << "<p>No hay bodegas registradas.</p>\n";
    }
    for (const auto& bodega : bodegas) {
        oss << seccionBodega(bodega);
    }
    return envolver("Productos", oss.str());
}

std::string PaginaWeb::error(int codigo, const std::string& mensaje) {
    std::string contenido = "<p>" + escapar(mensaje) + "</p>\n" +
                            "<p><a href=\"/\">Volver al inicio</a></p>\n";
    return envolver("Error " + std::to_string(codigo), contenido);
}

std::string PaginaWeb::seccionBodega(const BodegaVista& bodega) {
    std::ostringstream oss;
    oss << "<section>\n<h2>" << escapar(bodega.nombre)
        << " <small>(" << escapar(bodega.id) << ")</small></h2>\n";

    if (bodega.categorias.empty()) {
        oss << "<p>No hay categorías.</p>\n";
    }
    for (const auto& categoria : bodega.categorias) {
        // la categoria es una sola palabra (la del protocolo); el nombre aparte solo se
        // muestra si llega distinto, por ejemplo desde un servidor que mande id:nombre
        oss << "<h3>" << escapar(categoria.id);
        if (categoria.nombre != categoria.id) oss << " <small>(" << escapar(categoria.nombre) << ")</small>";
        oss << "</h3>\n" << tablaProductos(categoria);
    }
    oss << "</section>\n";
    return oss.str();
}

std::string PaginaWeb::tablaProductos(const CategoriaVista& categoria) {
    if (categoria.productos.empty()) {
        return "<p>No hay productos en esta categoría.</p>\n";
    }

    std::ostringstream oss;
    oss << "<table>\n<tr><th>Producto</th><th>Precio</th><th>Stock</th></tr>\n";
    for (const auto& p : categoria.productos) {
        oss << "<tr><td>" << escapar(p.nombre) << "</td><td>" << escapar(p.precio)
            << "</td><td>" << escapar(p.stock) << "</td></tr>\n";
    }
    oss << "</table>\n";
    return oss.str();
}

// pagina completa; el viewport hace que se adapte al ancho de un telefono
std::string PaginaWeb::envolver(const std::string& titulo, const std::string& contenido) {
    std::ostringstream oss;
    oss << "<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n"
        << "<meta charset=\"utf-8\">\n"
        << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        << "<title>" << escapar(titulo) << " - TicAmazon</title>\n"
        << "<style>" << ESTILOS << "</style>\n"
        << "</head>\n<body>\n"
        << "<header>TicAmazon</header>\n"
        << "<nav><a href=\"/\">Inicio</a>";
    for (const auto& opcion : OPCIONES) oss << "<a href=\"" << opcion.ruta << "\">" << opcion.nombre << "</a>";
    oss << "</nav>\n"
        << "<main>\n<h1>" << escapar(titulo) << "</h1>\n" << contenido << "</main>\n"
        << "</body>\n</html>\n";
    return oss.str();
}

std::string PaginaWeb::escapar(const std::string& texto) {
    std::string r;
    for (char c : texto) {
        switch (c) {
            case '&':  r += "&amp;";  break;
            case '<':  r += "&lt;";   break;
            case '>':  r += "&gt;";   break;
            case '"':  r += "&quot;"; break;
            case '\'': r += "&#39;";  break;
            default:   r += c;
        }
    }
    return r;
}