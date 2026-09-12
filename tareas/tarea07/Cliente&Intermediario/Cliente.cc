#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>

#include "Socket.h"
#include "Protocolo.h"
#include "HttpUtil.h"

static const char* HOST_SERVIDOR = "127.0.0.1"; // cambiar por la IP del intermediario en la isla
static const int PUERTO_SERVIDOR = 8080;
static const std::string MI_ID = "CLI_04";
static const std::string ID_INTERMEDIARIO = "INT_04";
static const int MAXLINE = 4096;

static std::vector<ItemCarrito> carrito;

std::string leerTodo(VSocket* s) {
    char buf[MAXLINE];
    std::string acumulado;
    try {
        int leidos;
        while ((leidos = s->Read(buf, MAXLINE - 1)) > 0) acumulado.append(buf, leidos);
    } catch (const std::exception&) {}
    return acumulado;
}

std::string enviarMensaje(const std::string& mensajeProtocolo) {
    VSocket* cliente = nullptr;
    try {
        cliente = new Socket('s');
        if (cliente->Connect(HOST_SERVIDOR, PUERTO_SERVIDOR) != 0) throw std::runtime_error("Connect fallo");

        std::string ruta = "/tic?msg=" + urlEncode(mensajeProtocolo);
        cliente->Write(buildGET(ruta, HOST_SERVIDOR).c_str());

        std::string respuesta = leerTodo(cliente);
        delete cliente;

        if (getCodigoEstado(respuesta) != 200) return "";
        return extraerCuerpo(respuesta);

    } catch (const std::exception& e) {
        std::cerr << "[Cliente] Error de comunicacion: " << e.what() << "\n";
        delete cliente;
        return "";
    }
}

std::string extraerAtributo(const std::string& tag, const std::string& nombre) {
    std::string buscado = nombre + "=\"";
    size_t pos = tag.find(buscado);
    if (pos == std::string::npos) return "";
    pos += buscado.size();
    size_t fin = tag.find("\"", pos);
    if (fin == std::string::npos) return "";
    return tag.substr(pos, fin - pos);
}

std::vector<std::string> extraerElementos(const std::string& html, const std::string& tag) {
    std::vector<std::string> elementos;
    std::string abre = "<" + tag + " ";
    size_t pos = 0;
    while ((pos = html.find(abre, pos)) != std::string::npos) {
        size_t fin = html.find(">", pos);
        if (fin == std::string::npos) break;
        elementos.push_back(html.substr(pos, fin - pos + 1));
        pos = fin + 1;
    }
    return elementos;
}

void comandoCategorias() {
    std::string msg = construirMensaje(MI_ID, ID_INTERMEDIARIO, TIPO_REQUEST_CATEGORY, {});
    std::cout << "[Cliente -> Servidor] " << msg << "\n";
    std::string html = enviarMensaje(msg);
    if (html.empty()) { std::cout << "[Servidor -> Cliente] ERR_COMM\n"; return; }
    for (auto& li : extraerElementos(html, "li")) {
        std::cout << "  " << extraerAtributo(li, "data-id") << "\n";
    }
}

void comandoProductos(const std::string& idCategoria) {
    std::string msg = construirMensaje(MI_ID, ID_INTERMEDIARIO, TIPO_REQUEST_PRODUCTS, {idCategoria});
    std::cout << "[Cliente -> Servidor] " << msg << "\n";
    std::string html = enviarMensaje(msg);
    if (html.empty()) { std::cout << "[Servidor -> Cliente] ERR_COMM\n"; return; }

    auto filas = extraerElementos(html, "tr");
    if (filas.empty()) { std::cout << "  (sin productos en '" << idCategoria << "')\n"; return; }
    for (auto& fila : filas) {
        std::cout << "  " << extraerAtributo(fila, "data-nombre")
                  << " | precio: " << extraerAtributo(fila, "data-precio")
                  << " | stock: " << extraerAtributo(fila, "data-stock") << "\n";
    }
}

void comandoDetalle(const std::string& nombre) {
    std::string msg = construirMensaje(MI_ID, ID_INTERMEDIARIO, TIPO_REQUEST_DETAIL, {nombre});
    std::cout << "[Cliente -> Servidor] " << msg << "\n";
    std::string html = enviarMensaje(msg);
    if (html.empty()) { std::cout << "[Servidor -> Cliente] ERR_COMM\n"; return; }

    if (html.find("data-tipo=\"" + std::to_string(TIPO_PRODUCT_NOT_FOUND) + "\"") != std::string::npos) {
        std::cout << "  " << nombre << " no encontrado\n";
        return;
    }
    std::cout << "  " << extraerAtributo(html, "data-nombre")
              << " | precio: " << extraerAtributo(html, "data-precio")
              << " | stock: " << extraerAtributo(html, "data-stock") << "\n";
}

void comandoAgregar(const std::string& args) {
    size_t espacio = args.find_last_of(' ');
    if (espacio == std::string::npos) { std::cout << "Uso: AGREGAR <nombre_producto> <cantidad>\n"; return; }
    std::string nombre = args.substr(0, espacio);
    std::string cantidadStr = args.substr(espacio + 1);

    std::string msg = construirMensaje(MI_ID, ID_INTERMEDIARIO, TIPO_ADD_TO_CART, {nombre, cantidadStr});
    std::cout << "[Cliente -> Servidor] " << msg << "\n";
    std::string html = enviarMensaje(msg);
    if (html.empty()) { std::cout << "[Servidor -> Cliente] ERR_COMM\n"; return; }

    if (html.find("data-tipo=\"" + std::to_string(TIPO_CART_ADDED) + "\"") != std::string::npos) {
        std::string precioStr = extraerAtributo(html, "data-precio");
        carrito.push_back({nombre, std::stoi(cantidadStr), std::stod(precioStr)});
        std::cout << "  agregado " << cantidadStr << " x " << nombre << " @ " << precioStr << "\n";
    } else if (html.find("data-tipo=\"" + std::to_string(TIPO_NO_STOCK) + "\"") != std::string::npos) {
        std::cout << "  stock insuficiente (disponible: " << extraerAtributo(html, "data-stock") << ")\n";
    } else {
        std::cout << "  " << nombre << " no encontrado\n";
    }
}

void comandoFactura() {
    std::ostringstream lista;
    for (size_t i = 0; i < carrito.size(); i++) {
        if (i > 0) lista << ";";
        lista << carrito[i].producto << "," << carrito[i].cantidad;
    }

    std::string msg = construirMensaje(MI_ID, ID_INTERMEDIARIO, TIPO_REQUEST_FACTURA,
                                        {std::to_string(carrito.size()), lista.str()});
    std::cout << "[Cliente -> Servidor] " << msg << "\n";
    std::string html = enviarMensaje(msg);
    if (html.empty()) { std::cout << "[Servidor -> Cliente] ERR_COMM\n"; return; }

    if (html.find("data-tipo=\"" + std::to_string(TIPO_CARRITO_VACIO) + "\"") != std::string::npos) {
        std::cout << "  el carrito esta vacio\n";
        return;
    }

    std::cout << "\n--- Factura TicAmazon ---\n";
    for (auto& fila : extraerElementos(html, "tr")) {
        std::cout << "  " << extraerAtributo(fila, "data-producto")
                  << " x" << extraerAtributo(fila, "data-cantidad")
                  << " = " << extraerAtributo(fila, "data-subtotal") << "\n";
    }
    std::cout << "TOTAL: " << extraerAtributo(html, "data-total") << "\n";
    carrito.clear();
}

int main() {
    std::cout << "---TicAmazon Cliente---\n";
    std::cout << "Comandos: CATEGORIAS | <idCategoria> | PRODUCTO <nombre> | "
                 "AGREGAR <nombre> <cantidad> | FACTURA | salir\n";

    std::string linea;
    while (std::cout << "\n> " && std::getline(std::cin, linea)) {
        std::string comando = linea;
        for (char& c : comando) c = (char)tolower((unsigned char)c);

        if (comando == "salir") break;
        if (comando == "categorias") { comandoCategorias(); continue; }
        if (comando.rfind("producto ", 0) == 0) { comandoDetalle(linea.substr(9)); continue; }
        if (comando.rfind("agregar ", 0) == 0) { comandoAgregar(linea.substr(8)); continue; }
        if (comando == "factura") { comandoFactura(); continue; }

        comandoProductos(linea); // cualquier otra cosa se trata como id de categoria
    }

    std::cout << "\n---Fin---\n";
    return 0;
}