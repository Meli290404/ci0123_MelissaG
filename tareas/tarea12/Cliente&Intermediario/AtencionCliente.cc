#include "AtencionCliente.h"
#include "Validaciones.h"
#include <iostream>
#include <sstream>
#include <cstdio>

static std::vector<std::string> separar(const std::string& texto, char separador) {
    std::vector<std::string> partes;
    std::stringstream ss(texto);
    std::string parte;
    while (std::getline(ss, parte, separador)) {
        if (!parte.empty()) partes.push_back(parte);
    }
    return partes;
}

static bool esNumero(const std::string& texto) {
    if (texto.empty() || texto.size() > 9) return false;
    for (char c : texto) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

// precio con el formato que valida validarPrecio, por ejemplo "1500.00" o "3.5", a centavos
static uint32_t textoACentavos(const std::string& precio) {
    size_t punto = precio.find('.');
    uint32_t entero = (uint32_t)std::stoul(precio.substr(0, punto));
    uint32_t decimales = 0;
    if (punto != std::string::npos) {
        std::string parte = precio.substr(punto + 1);
        if (parte.size() == 1) parte += "0";
        decimales = (uint32_t)std::stoul(parte);
    }
    return entero * 100 + decimales;
}

// el total de una factura puede pasar de uint32_t, por eso este formato usa uint64_t
static std::string centavosATexto(uint64_t centavos) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu.%02llu",
             (unsigned long long)(centavos / 100), (unsigned long long)(centavos % 100));
    return buf;
}

AtencionCliente::AtencionCliente(ConexionBodega& bodega, Carritos& carritos)
    : bodega_(bodega), carritos_(carritos) {}

RespuestaHttp AtencionCliente::responder(const std::string& peticion) {
    if (peticion.rfind("GET ", 0) != 0) {
        return error(405, "error", "Este servidor solo acepta peticiones GET.");
    }

    std::string ruta = extraerRuta(peticion);
    if (ruta == "/") return inicio();
    if (ruta == "/sesion") return sesion();
    if (ruta == "/categorias") return categorias();
    if (ruta == "/productos") return productos(parametroUrl(peticion, "categoria"));
    if (ruta == "/agregar") {
        return agregar(parametroUrl(peticion, "sesion"), parametroUrl(peticion, "producto"),
                       parametroUrl(peticion, "cantidad"));
    }
    if (ruta == "/factura") return factura(parametroUrl(peticion, "sesion"));

    return error(404, "ruta-inexistente", "La página " + ruta + " no existe.");
}

// pagina del navegador: pide las categorias (10) y los productos de cada una (20)
RespuestaHttp AtencionCliente::inicio() {
    Mensaje lista;
    RespuestaHttp fallo;
    if (!consultar(TIPO_REQUEST_CATEGORIES, {}, lista, fallo)) return fallo;
    if (lista.tipo != TIPO_CATEGORY_LIST) {
        return error(502, "respuesta-invalida", "El servidor de productos respondió un mensaje inesperado.");
    }

    // el protocolo no separa por bodega, asi que toda la isla se muestra como una sola seccion
    BodegaVista isla;
    isla.id = ID_INTERMEDIARIO;
    isla.nombre = "Isla 4";

    for (const auto& id : separar(lista.campos.empty() ? "" : lista.campos[0], ',')) {
        if (!validarCategoria(id)) continue;

        CategoriaVista categoria;
        categoria.id = id;
        categoria.nombre = id;

        Mensaje productosMsg;
        if (!consultar(TIPO_REQUEST_PRODUCTS, {id}, productosMsg, fallo)) return fallo;
        leerListaProductos(productosMsg, categoria.productos);
        isla.categorias.push_back(categoria);
    }

    return {200, pagina_.inventario({isla})};
}

RespuestaHttp AtencionCliente::sesion() {
    int nueva = carritos_.nuevaSesion();
    std::cout << "[INT] sesion nueva: " << nueva << "\n";
    return respuesta(200, "Sesión", "ok", "data-sesion=\"" + std::to_string(nueva) + "\"",
                     "<p>Su número de sesión es " + std::to_string(nueva) + ".</p>\n");
}

RespuestaHttp AtencionCliente::categorias() {
    Mensaje lista;
    RespuestaHttp fallo;
    if (!consultar(TIPO_REQUEST_CATEGORIES, {}, lista, fallo)) return fallo;
    if (lista.tipo != TIPO_CATEGORY_LIST) {
        return error(502, "respuesta-invalida", "El servidor de productos respondió un mensaje inesperado.");
    }

    std::ostringstream oss;
    oss << "<ul class=\"categorias\">\n";
    for (const auto& id : separar(lista.campos.empty() ? "" : lista.campos[0], ',')) {
        if (!validarCategoria(id)) continue;
        oss << "<li data-id=\"" << pagina_.escapar(id) << "\">" << pagina_.escapar(id) << "</li>\n";
    }
    oss << "</ul>\n";
    return respuesta(200, "Categorías", "ok", "", oss.str());
}

RespuestaHttp AtencionCliente::productos(const std::string& categoria) {
    if (!validarCategoria(categoria)) {
        return error(400, "categoria-invalida", "La categoría debe tener entre 3 y 20 letras.");
    }

    Mensaje lista;
    RespuestaHttp fallo;
    if (!consultar(TIPO_REQUEST_PRODUCTS, {categoria}, lista, fallo)) return fallo;

    std::vector<ProductoVista> vistas;
    if (!leerListaProductos(lista, vistas)) {
        return error(502, "respuesta-invalida", "El servidor de productos respondió una lista mal formada.");
    }

    // 22 con count 0: la categoria no existe o no tiene productos
    if (vistas.empty()) {
        return respuesta(200, "Productos", "sin-productos", "data-categoria=\"" + pagina_.escapar(categoria) + "\"",
                         "<p>No hay productos disponibles en la categoría \"" + pagina_.escapar(categoria) + "\".</p>\n");
    }

    std::ostringstream oss;
    oss << "<table class=\"productos\">\n<tr><th>Producto</th><th>Precio</th><th>Stock</th></tr>\n";
    for (const auto& p : vistas) {
        oss << "<tr data-nombre=\"" << pagina_.escapar(p.nombre) << "\" data-precio=\"" << pagina_.escapar(p.precio)
            << "\" data-stock=\"" << pagina_.escapar(p.stock) << "\"><td>" << pagina_.escapar(p.nombre)
            << "</td><td>" << pagina_.escapar(p.precio) << "</td><td>" << pagina_.escapar(p.stock) << "</td></tr>\n";
    }
    oss << "</table>\n";
    return respuesta(200, "Productos de " + categoria, "ok", "", oss.str());
}

RespuestaHttp AtencionCliente::agregar(const std::string& sesionTexto, const std::string& producto,
                                       const std::string& cantidadTexto) {
    int sesionId;
    if (!leerSesion(sesionTexto, sesionId)) {
        return error(400, "sesion-invalida", "La sesión no existe; pida una nueva en /sesion.");
    }
    if (!validarProducto(producto)) {
        return error(400, "producto-invalido", "El producto solo puede tener letras, dígitos o guion.");
    }
    if (!validarCount(cantidadTexto) || std::stoi(cantidadTexto) == 0) {
        return error(400, "cantidad-invalida", "La cantidad debe estar entre 1 y 999.");
    }

    Mensaje reserva;
    RespuestaHttp fallo;
    if (!consultar(TIPO_RESERVE_STOCK, {producto, cantidadTexto}, reserva, fallo)) return fallo;

    if (reserva.tipo == TIPO_PRODUCT_NOT_FOUND) {
        return respuesta(200, "Agregar", "no-existe", "data-producto=\"" + pagina_.escapar(producto) + "\"",
                         "<p>\"" + pagina_.escapar(producto) + "\" no existe.</p>\n");
    }

    if (reserva.tipo != TIPO_RESERVED || reserva.campos.size() < 3 || reserva.campos[0] != producto ||
        !validarPrecio(reserva.campos[1]) || !validarCount(reserva.campos[2])) {
        return error(502, "respuesta-invalida", "El servidor de productos respondió una reserva mal formada.");
    }

    std::string precio = reserva.campos[1];
    int reservado = std::stoi(reserva.campos[2]);

    if (reservado == 0) {
        return respuesta(200, "Agregar", "sin-stock", "data-producto=\"" + pagina_.escapar(producto) + "\"",
                         "<p>No hay stock de " + pagina_.escapar(producto) + ".</p>\n");
    }

    carritos_.agregar(sesionId, {producto, reservado, textoACentavos(precio)});

    // la cantidad reservada puede ser menor a la pedida si no habia suficiente stock
    std::string atributos = "data-producto=\"" + pagina_.escapar(producto) + "\" data-precio=\"" + precio +
                            "\" data-cantidad=\"" + std::to_string(reservado) + "\" data-pedido=\"" + cantidadTexto + "\"";
    return respuesta(200, "Agregar", "agregado", atributos,
                     "<p>Agregado " + std::to_string(reservado) + " x " + pagina_.escapar(producto) +
                     " @ " + precio + "</p>\n");
}

// arma la factura con lo ya reservado en el carrito, sin volver a consultar al servidor de productos
RespuestaHttp AtencionCliente::factura(const std::string& sesionTexto) {
    int sesionId;
    if (!leerSesion(sesionTexto, sesionId)) {
        return error(400, "sesion-invalida", "La sesión no existe; pida una nueva en /sesion.");
    }

    std::vector<ItemReservado> items;
    carritos_.vaciar(sesionId, items);
    if (items.empty()) {
        return respuesta(200, "Factura", "vacio", "", "<p>El carrito está vacío.</p>\n");
    }

    uint64_t total = 0;
    std::string listaProtocolo;
    std::ostringstream filas;
    filas << "<table class=\"factura\">\n<tr><th>Producto</th><th>Cantidad</th><th>Precio</th><th>Subtotal</th></tr>\n";

    for (const auto& item : items) {
        uint64_t subtotal = (uint64_t)item.precioCentavos * item.cantidad;
        total += subtotal;

        filas << "<tr data-producto=\"" << pagina_.escapar(item.producto) << "\" data-cantidad=\"" << item.cantidad
              << "\" data-subtotal=\"" << centavosATexto(subtotal) << "\"><td>" << pagina_.escapar(item.producto)
              << "</td><td>" << item.cantidad << "</td><td>" << centavosATexto(item.precioCentavos)
              << "</td><td>" << centavosATexto(subtotal) << "</td></tr>\n";

        if (!listaProtocolo.empty()) listaProtocolo += ";";
        listaProtocolo += item.producto + "," + std::to_string(item.cantidad) + "," + centavosATexto(subtotal);
    }
    filas << "</table>\n<p>Total: " << centavosATexto(total) << "</p>\n";

    // el registro usa el mismo formato del mensaje 41: total, cantidad de productos y detalle
    std::cout << "[INT] factura sesion " << sesionId << ": 41/" << centavosATexto(total) << "/"
              << items.size() << "/" << listaProtocolo << "\n";

    return respuesta(200, "Factura proforma", "ok", "data-total=\"" + centavosATexto(total) + "\"", filas.str());
}

bool AtencionCliente::consultar(int tipo, const std::vector<std::string>& campos, Mensaje& respuestaMsg,
                                RespuestaHttp& fallo) {
    std::string enviado = construirMensaje(ID_INTERMEDIARIO, ID_SERVIDOR, tipo, campos);
    std::cout << "[INT -> SERV] " << enviado << "\n";

    std::string texto;
    int reintentos = 0;
    if (!bodega_.enviar(enviado, texto, reintentos)) {
        // mismos datos que el mensaje 92: componente que no responde, tipo pendiente y reintentos
        std::cout << "[INT] sin respuesta de " << ID_SERVIDOR << " al tipo " << tipo
                  << " despues de " << reintentos << " reintentos\n";
        std::string atributos = "data-componente=\"" + ID_SERVIDOR + "\" data-tipo-pendiente=\"" +
                                std::to_string(tipo) + "\" data-intentos=\"" + std::to_string(reintentos) + "\"";
        fallo = respuesta(502, "Error de comunicación", "sin-comunicacion", atributos,
                          "<p>El servidor de productos no responde.</p>\n");
        return false;
    }
    std::cout << "[SERV -> INT] " << texto << "\n";

    try {
        respuestaMsg = parsearMensaje(texto);
    } catch (const std::exception&) {
        fallo = error(502, "respuesta-invalida", "El servidor de productos respondió un mensaje mal formado.");
        return false;
    }

    if (respuestaMsg.origen != ID_SERVIDOR) {
        fallo = error(502, "respuesta-invalida", "La respuesta no viene del servidor de productos.");
        return false;
    }
    if (respuestaMsg.tipo == TIPO_ERR_FORMATO || respuestaMsg.tipo == TIPO_ERR_TAMANO) {
        fallo = error(502, "error-servidor", "El servidor de productos rechazó el mensaje: " + texto);
        return false;
    }
    return true;
}

bool AtencionCliente::leerListaProductos(const Mensaje& mensaje, std::vector<ProductoVista>& productosVista) {
    productosVista.clear();
    if (mensaje.tipo != TIPO_PRODUCT_LIST || mensaje.campos.empty() || !validarCount(mensaje.campos[0])) return false;
    if (mensaje.campos[0] == "0") return true;
    if (mensaje.campos.size() < 2) return false;

    // cada producto viene como nombre,precio,stock y se separan con ';'
    for (const auto& item : separar(mensaje.campos[1], ';')) {
        std::vector<std::string> partes = separar(item, ',');
        if (partes.size() != 3 || !validarProducto(partes[0]) || !validarPrecio(partes[1]) || !validarStock(partes[2])) {
            return false;
        }
        productosVista.push_back({partes[0], partes[1], partes[2]});
    }
    return true;
}

bool AtencionCliente::leerSesion(const std::string& sesionTexto, int& sesionId) {
    if (!esNumero(sesionTexto)) return false;
    sesionId = std::stoi(sesionTexto);
    return carritos_.existe(sesionId);
}

// el div con class respuesta y data-estado es lo que lee el cliente de terminal;
// el resto de la pagina es para que tambien se vea bien en un navegador
RespuestaHttp AtencionCliente::respuesta(int codigo, const std::string& titulo, const std::string& estado,
                                         const std::string& atributos, const std::string& contenido) {
    std::string div = "<div class=\"respuesta\" data-estado=\"" + estado + "\"" +
                      (atributos.empty() ? "" : " " + atributos) + ">\n" + contenido + "</div>\n";
    return {codigo, pagina_.envolver(titulo, div)};
}

RespuestaHttp AtencionCliente::error(int codigo, const std::string& estado, const std::string& detalle) {
    return respuesta(codigo, "Error " + std::to_string(codigo), estado,
                     "data-detalle=\"" + pagina_.escapar(detalle) + "\"",
                     "<p>" + pagina_.escapar(detalle) + "</p>\n");
}