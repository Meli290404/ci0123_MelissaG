#include "AtencionCliente.h"
#include "Validaciones.h"
#include "ConexionBodega.h"
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

AtencionCliente::AtencionCliente(TablaRutas& rutas, Carritos& carritos)
    : rutas_(rutas), carritos_(carritos) {}

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
    if (ruta == "/rutas") return paginaRutas();

    return error(404, "ruta-inexistente", "La página " + ruta + " no existe.");
}

// pagina del navegador: una seccion por servidor de productos de la tabla de rutas, con sus
// categorias (10) y los productos de cada una (20)
RespuestaHttp AtencionCliente::inicio() {
    std::vector<ServidorRuta> servidores = rutas_.activos();
    if (servidores.empty()) return sinServidores();

    std::vector<BodegaVista> secciones;
    for (const auto& servidor : servidores) {
        BodegaVista seccion;
        seccion.id = servidor.id;
        seccion.nombre = servidor.id + " (" + servidor.ip + ":" + std::to_string(servidor.puerto) + ")";

        bool contesto = false;
        for (auto& categoria : categoriasDe(servidor, contesto)) {
            Mensaje productosMsg;
            if (consultar(servidor, TIPO_REQUEST_PRODUCTS, {categoria.id}, productosMsg)) {
                leerListaProductos(productosMsg, categoria.productos);
            }
            seccion.categorias.push_back(categoria);
        }
        if (contesto) secciones.push_back(seccion);
    }
    if (secciones.empty()) return sinComunicacion();
    return {200, pagina_.inventario(secciones)};
}

RespuestaHttp AtencionCliente::sesion() {
    int nueva = carritos_.nuevaSesion();
    std::cout << "[INT] sesion nueva: " << nueva << "\n";
    return respuesta(200, "Sesión", "ok", "data-sesion=\"" + std::to_string(nueva) + "\"",
                     "<p>Su número de sesión es " + std::to_string(nueva) + ".</p>\n");
}

std::vector<CategoriaVista> AtencionCliente::categoriasDe(const ServidorRuta& servidor, bool& contesto) {
    Mensaje lista;
    contesto = consultar(servidor, TIPO_REQUEST_CATEGORIES, {}, lista) && lista.tipo == TIPO_CATEGORY_LIST;
    if (!contesto) return {};
    return leerListaCategorias(lista);
}

// el cliente ve una sola lista: las categorias de todos los servidores activos, sin repetir
RespuestaHttp AtencionCliente::categorias() {
    std::vector<ServidorRuta> servidores = rutas_.activos();
    if (servidores.empty()) return sinServidores();

    std::vector<CategoriaVista> todas;
    bool alguno = false;
    for (const auto& servidor : servidores) {
        bool contesto = false;
        for (const auto& categoria : categoriasDe(servidor, contesto)) {
            bool repetida = false;
            for (const auto& c : todas) {
                if (c.id == categoria.id) repetida = true;
            }
            if (!repetida) todas.push_back(categoria);
        }
        alguno = alguno || contesto;
    }
    if (!alguno) return sinComunicacion();

    std::ostringstream oss;
    oss << "<ul class=\"categorias\">\n";
    for (const auto& categoria : todas) {
        oss << "<li data-id=\"" << pagina_.escapar(categoria.id) << "\" data-nombre=\"" << pagina_.escapar(categoria.nombre)
            << "\">" << pagina_.escapar(categoria.nombre) << "</li>\n";
    }
    oss << "</ul>\n";
    return respuesta(200, "Categorías", "ok", "", oss.str());
}

RespuestaHttp AtencionCliente::productos(const std::string& categoria) {
    if (!validarCategoria(categoria)) {
        return error(400, "categoria-invalida", "La categoría debe tener entre 3 y 20 letras.");
    }

    std::vector<ServidorRuta> activos = rutas_.activos();
    if (activos.empty()) return sinServidores();

    // primero los servidores que anunciaron la categoria en su 80 u 83 (la ruta). si ninguno
    // tiene productos se pregunta a los demas, por si la categoria se agrego despues del ultimo 83
    std::vector<ServidorRuta> conRuta = rutas_.conCategoria(categoria);
    std::vector<ServidorRuta> resto;
    for (const auto& s : activos) {
        bool tieneRuta = false;
        for (const auto& r : conRuta) {
            if (r.id == s.id) tieneRuta = true;
        }
        if (!tieneRuta) resto.push_back(s);
    }
    if (conRuta.empty()) std::cout << "[INT] ningun servidor anuncio " << categoria << ", se pregunta a todos\n";

    std::vector<ProductoVista> vistas;
    bool alguno = false;
    auto preguntar = [&](const ServidorRuta& servidor) {
        Mensaje lista;
        if (!consultar(servidor, TIPO_REQUEST_PRODUCTS, {categoria}, lista)) return;
        alguno = true;
        std::vector<ProductoVista> deEste;
        if (!leerListaProductos(lista, deEste)) return;
        // si el producto ya vino de otro servidor se suma su stock: agregar lo puede sacar de los dos
        for (const auto& p : deEste) {
            bool repetido = false;
            for (auto& v : vistas) {
                if (v.nombre == p.nombre) {
                    v.stock = std::to_string(std::stoi(v.stock) + std::stoi(p.stock));
                    repetido = true;
                }
            }
            if (!repetido) vistas.push_back(p);
        }
    };

    for (const auto& servidor : conRuta) preguntar(servidor);
    for (size_t i = 0; vistas.empty() && i < resto.size(); i++) preguntar(resto[i]);
    if (!alguno) return sinComunicacion();

    // 22 con count 0 en todos: la categoria no existe o no tiene productos
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

    std::vector<ServidorRuta> servidores = rutas_.activos();
    if (servidores.empty()) return sinServidores();

    // se pide la reserva (50) a un servidor; si no tiene el producto (27) o no le alcanza el stock,
    // se le pide lo que falta al siguiente servidor activo, hasta completar o recorrerlos todos
    int pedido = std::stoi(cantidadTexto);
    int faltan = pedido;
    bool encontrado = false;
    bool alguienContesto = false;
    std::string primerPrecio;

    for (const auto& servidor : servidores) {
        if (faltan == 0) break;

        Mensaje reserva;
        if (!consultar(servidor, TIPO_RESERVE_STOCK, {producto, std::to_string(faltan)}, reserva)) continue;
        alguienContesto = true;

        if (reserva.tipo == TIPO_PRODUCT_NOT_FOUND) {
            std::cout << "[INT] " << servidor.id << " no tiene " << producto << "\n";
            continue;
        }
        if (reserva.tipo != TIPO_RESERVED || reserva.campos.size() < 3 || reserva.campos[0] != producto ||
            !validarPrecio(reserva.campos[1]) || !validarCount(reserva.campos[2])) {
            std::cout << "[INT] " << servidor.id << " respondio una reserva mal formada\n";
            continue;
        }

        encontrado = true;
        int reservado = std::stoi(reserva.campos[2]);
        if (reservado > 0) {
            carritos_.agregar(sesionId, {producto, reservado, textoACentavos(reserva.campos[1])});
            if (primerPrecio.empty()) primerPrecio = reserva.campos[1];
            faltan -= reservado;
        }
        if (faltan > 0) {
            std::cout << "[INT] " << servidor.id << " reservo " << reservado << " de " << producto
                      << ", faltan " << faltan << "\n";
        }
    }

    if (!alguienContesto) return sinComunicacion();
    if (!encontrado) {
        return respuesta(200, "Agregar", "no-existe", "data-producto=\"" + pagina_.escapar(producto) + "\"",
                         "<p>\"" + pagina_.escapar(producto) + "\" no existe.</p>\n");
    }

    int reservadoTotal = pedido - faltan;
    if (reservadoTotal == 0) {
        return respuesta(200, "Agregar", "sin-stock", "data-producto=\"" + pagina_.escapar(producto) + "\"",
                         "<p>No hay stock de " + pagina_.escapar(producto) + ".</p>\n");
    }

    // la cantidad reservada puede ser menor a la pedida si entre todos los servidores no alcanzo
    std::string atributos = "data-producto=\"" + pagina_.escapar(producto) + "\" data-precio=\"" + primerPrecio +
                            "\" data-cantidad=\"" + std::to_string(reservadoTotal) + "\" data-pedido=\"" + cantidadTexto + "\"";
    return respuesta(200, "Agregar", "agregado", atributos,
                     "<p>Agregado " + std::to_string(reservadoTotal) + " x " + pagina_.escapar(producto) +
                     " @ " + primerPrecio + "</p>\n");
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

// tabla de rutas: una fila por servidor descubierto y una fila por cada categoria que anuncio
RespuestaHttp AtencionCliente::paginaRutas() {
    std::vector<ServidorRuta> servidores = rutas_.todos();

    std::ostringstream oss;
    oss << "<meta http-equiv=\"refresh\" content=\"2\">\n";
    oss << "<h2>Servidores de productos</h2>\n<table class=\"servidores\">\n"
        << "<tr><th>Servidor</th><th>Dirección</th><th>Estado</th><th>Último contacto</th><th>Categorías</th></tr>\n";
    for (const auto& s : servidores) {
        oss << "<tr data-id=\"" << s.id << "\" data-estado=\"" << TablaRutas::estado(s) << "\"><td>" << s.id
            << "</td><td>" << s.ip << ":" << s.puerto << "</td><td>" << TablaRutas::estado(s) << "</td><td>hace "
            << (time(nullptr) - s.ultimoContacto) << " s</td><td>" << s.categorias.size() << "</td></tr>\n";
    }
    oss << "</table>\n";

    oss << "<h2>Rutas por categoría</h2>\n<table class=\"rutas\">\n"
        << "<tr><th>Categoría</th><th>Servidor</th><th>Dirección</th><th>Estado</th></tr>\n";
    int filas = 0;
    for (const auto& s : servidores) {
        for (const auto& c : s.categorias) {
            // si otro servidor anuncio la categoria antes, esta fila no es la ruta que se usa
            std::string estado = TablaRutas::estado(s);
            std::vector<ServidorRuta> dueno = rutas_.conCategoria(c);
            if (!dueno.empty() && dueno[0].id != s.id) estado = "repetida (ruta de " + dueno[0].id + ")";

            oss << "<tr data-categoria=\"" << pagina_.escapar(c) << "\" data-servidor=\"" << s.id << "\" data-estado=\"" << estado
                << "\"><td>" << pagina_.escapar(c) << "</td><td>" << s.id << "</td><td>" << s.ip << ":" << s.puerto
                << "</td><td>" << estado << "</td></tr>\n";
            filas++;
        }
    }
    oss << "</table>\n";
    if (servidores.empty()) oss << "<p>Todavía no se ha descubierto ningún servidor de productos.</p>\n";

    return respuesta(200, "Tabla de rutas", "ok",
                     "data-servidores=\"" + std::to_string(servidores.size()) + "\" data-rutas=\"" +
                     std::to_string(filas) + "\"", oss.str());
}

RespuestaHttp AtencionCliente::sinServidores() {
    std::cout << "[INT] la tabla de rutas no tiene servidores activos\n";
    return respuesta(503, "Sin servidores", "sin-servidores", "",
                     "<p>Todavía no se ha descubierto ningún servidor de productos.</p>\n");
}

RespuestaHttp AtencionCliente::sinComunicacion() {
    return respuesta(502, "Error de comunicación", "sin-comunicacion", "",
                     "<p>Ningún servidor de productos responde.</p>\n");
}

bool AtencionCliente::consultar(const ServidorRuta& servidor, int tipo, const std::vector<std::string>& campos,
                                Mensaje& respuestaMsg) {
    std::string enviado = construirMensaje(ID_INTERMEDIARIO, servidor.id, tipo, campos);
    std::cout << "[INT -> " << servidor.id << "] " << enviado << "\n";

    std::string texto;
    int reintentos = 0;
    ConexionBodega conexion(servidor.ip, servidor.puerto);
    if (!conexion.enviar(enviado, texto, reintentos)) {
        // mismos datos que el mensaje 92: componente que no responde, tipo pendiente y reintentos
        std::cout << "[INT] sin respuesta de " << servidor.id << " al tipo " << tipo << " despues de "
                  << reintentos << " reintentos, sale de la tabla de rutas hasta su proximo 83\n";
        rutas_.marcarSinRespuesta(servidor.id);
        return false;
    }
    std::cout << "[" << servidor.id << " -> INT] " << texto << "\n";

    try {
        respuestaMsg = parsearMensaje(texto);
    } catch (const std::exception&) {
        std::cout << "[INT] " << servidor.id << " respondio un mensaje mal formado\n";
        return false;
    }
    if (respuestaMsg.origen != servidor.id) {
        std::cout << "[INT] la respuesta no viene de " << servidor.id << "\n";
        return false;
    }
    if (respuestaMsg.tipo == TIPO_ERR_FORMATO || respuestaMsg.tipo == TIPO_ERR_TAMANO) {
        std::cout << "[INT] " << servidor.id << " rechazo el mensaje: " << texto << "\n";
        return false;
    }
    rutas_.marcarRespondio(servidor.id);
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

std::vector<CategoriaVista> AtencionCliente::leerListaCategorias(const Mensaje& mensaje) {
    std::vector<CategoriaVista> categorias;
    for (const auto& item : separar(mensaje.campos.empty() ? "" : mensaje.campos[0], ',')) {
        // si una categoria llega sin ':' se usa el id tambien como nombre
        size_t dosPuntos = item.find(':');
        CategoriaVista categoria;
        categoria.id = item.substr(0, dosPuntos);
        categoria.nombre = (dosPuntos == std::string::npos) ? categoria.id : item.substr(dosPuntos + 1);
        if (!validarCategoria(categoria.id) || categoria.nombre.empty()) continue;
        categorias.push_back(categoria);
    }
    return categorias;
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