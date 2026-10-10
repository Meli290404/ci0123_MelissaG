#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

#include "Socket.h"
#include "HttpUtil.h"

static const int PUERTO_INTERMEDIARIO = 8080;
static const int MAXLINE = 4096;

static std::string hostIntermediario;   // se recibe por argumento
static std::string sesion;              // numero que reparte el intermediario en /sesion

std::string leerTodo(VSocket* s) {
    char buf[MAXLINE];
    std::string acumulado;
    try {
        size_t leidos;
        while ((leidos = s->Read(buf, MAXLINE - 1)) > 0) acumulado.append(buf, leidos);
    } catch (const std::exception&) {}
    return acumulado;
}

// hace un get al intermediario y devuelve el cuerpo; codigo queda en -1 si no hubo conexion
std::string pedir(const std::string& ruta, int& codigo) {
    std::cout << "[Cliente -> Intermediario] GET " << ruta << "\n";
    VSocket* conexion = nullptr;
    try {
        conexion = new Socket('s');
        conexion->Connect(hostIntermediario.c_str(), PUERTO_INTERMEDIARIO);
        conexion->Write(buildGET(ruta, hostIntermediario).c_str());

        std::string respuesta = leerTodo(conexion);
        delete conexion;

        codigo = getCodigoEstado(respuesta);
        return extraerCuerpo(respuesta);

    } catch (const std::exception& e) {
        std::cerr << "[Cliente] error de comunicacion: " << e.what() << "\n";
        delete conexion;
        codigo = -1;
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

// muestra los errores que no son propios de cada comando
void mostrarError(const std::string& html, int codigo) {
    if (codigo == -1) {
        std::cout << "  no se pudo conectar con el intermediario\n";
        return;
    }
    std::string estado = extraerAtributo(html, "data-estado");
    if (estado == "sin-comunicacion") {
        std::cout << "  el servidor de productos no responde (" << extraerAtributo(html, "data-intentos")
                  << " reintentos)\n";
        return;
    }
    std::string detalle = extraerAtributo(html, "data-detalle");
    std::cout << "  error " << codigo << ": " << (detalle.empty() ? estado : detalle) << "\n";
}

bool obtenerSesion() {
    int codigo;
    std::string html = pedir("/sesion", codigo);
    sesion = (codigo == 200) ? extraerAtributo(html, "data-sesion") : "";
    if (sesion.empty()) {
        mostrarError(html, codigo);
        return false;
    }
    std::cout << "  sesion " << sesion << "\n";
    return true;
}

// pide la ruta y, si el intermediario ya no reconoce la sesion, pide una nueva y repite una vez
std::string pedirConSesion(const std::string& rutaSinSesion, int& codigo) {
    std::string separador = (rutaSinSesion.find('?') == std::string::npos) ? "?" : "&";
    std::string html = pedir(rutaSinSesion + separador + "sesion=" + sesion, codigo);

    if (extraerAtributo(html, "data-estado") == "sesion-invalida") {
        std::cout << "  la sesion ya no existe en el intermediario, se pide una nueva\n";
        if (!obtenerSesion()) return html;
        html = pedir(rutaSinSesion + separador + "sesion=" + sesion, codigo);
    }
    return html;
}

void comandoCategorias() {
    int codigo;
    std::string html = pedir("/categorias", codigo);
    if (codigo != 200) { mostrarError(html, codigo); return; }

    // el id es lo que se escribe para pedir los productos de la categoria
    for (auto& li : extraerElementos(html, "li")) {
        std::cout << "  " << extraerAtributo(li, "data-id") << "  " << extraerAtributo(li, "data-nombre") << "\n";
    }
}

void comandoProductos(const std::string& idCategoria) {
    int codigo;
    std::string html = pedir("/productos?categoria=" + urlEncode(idCategoria), codigo);
    if (codigo != 200) { mostrarError(html, codigo); return; }

    if (extraerAtributo(html, "data-estado") == "sin-productos") {
        std::cout << "  no hay productos disponibles en la categoria \"" << idCategoria << "\"\n";
        return;
    }
    for (auto& fila : extraerElementos(html, "tr")) {
        std::cout << "  " << extraerAtributo(fila, "data-nombre")
                  << " | precio: " << extraerAtributo(fila, "data-precio")
                  << " | stock: " << extraerAtributo(fila, "data-stock") << "\n";
    }
}

void comandoAgregar(const std::string& args) {
    size_t espacio = args.find_last_of(' ');
    if (espacio == std::string::npos) { std::cout << "  uso: AGREGAR <nombre_producto> <cantidad>\n"; return; }
    std::string nombre = args.substr(0, espacio);
    std::string cantidad = args.substr(espacio + 1);

    int codigo;
    std::string html = pedirConSesion("/agregar?producto=" + urlEncode(nombre) + "&cantidad=" + urlEncode(cantidad), codigo);
    if (codigo != 200) { mostrarError(html, codigo); return; }

    std::string estado = extraerAtributo(html, "data-estado");
    if (estado == "agregado") {
        std::string reservado = extraerAtributo(html, "data-cantidad");
        std::string pedido = extraerAtributo(html, "data-pedido");
        std::cout << "  agregado " << reservado << " x " << nombre << " @ " << extraerAtributo(html, "data-precio") << "\n";
        if (reservado != pedido) {
            std::cout << "  solo habia " << reservado << " disponibles de los " << pedido << " pedidos\n";
        }
    } else if (estado == "sin-stock") {
        std::cout << "  no hay stock de " << nombre << "\n";
    } else if (estado == "no-existe") {
        std::cout << "  " << nombre << " no existe\n";
    } else {
        mostrarError(html, codigo);
    }
}

void comandoFactura() {
    int codigo;
    std::string html = pedirConSesion("/factura", codigo);
    if (codigo != 200) { mostrarError(html, codigo); return; }

    if (extraerAtributo(html, "data-estado") == "vacio") {
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
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "uso: " << argv[0] << " <ip del intermediario>\n";
        return 1;
    }
    hostIntermediario = argv[1];

    std::cout << "---TicAmazon Cliente---\n";
    if (!obtenerSesion()) {
        std::cerr << "no se pudo obtener una sesion del intermediario\n";
        return 1;
    }
    std::cout << "Comandos: CATEGORIAS | <idCategoria> | AGREGAR <nombre> <cantidad> | FACTURA | salir\n";

    std::string linea;
    while (std::cout << "\n> " && std::getline(std::cin, linea)) {
        std::string comando = linea;
        for (char& c : comando) c = (char)tolower((unsigned char)c);

        if (comando == "salir") break;
        if (comando.empty()) continue;
        if (comando == "categorias") { comandoCategorias(); continue; }
        if (comando.rfind("agregar ", 0) == 0) { comandoAgregar(linea.substr(8)); continue; }
        if (comando == "factura") { comandoFactura(); continue; }

        comandoProductos(linea); // cualquier otra cosa se trata como id de categoria
    }

    std::cout << "\n---Fin---\n";
    return 0;
}