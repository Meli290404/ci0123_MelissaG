#include "AtencionCliente.h"
#include "Validaciones.h"
#include "ConsultaServidor.h"
#include "ConexionIsla.h"
#include "Reservas.h"
#include "Mancomunado.h"
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

AtencionCliente::AtencionCliente(TablaRutas& rutas, TablaIslas& islas, Carritos& carritos)
    : rutas_(rutas), islas_(islas), carritos_(carritos) {}

RespuestaHttp AtencionCliente::responder(const std::string& peticion) {
    if (peticion.rfind("GET ", 0) != 0) {
        return error(405, "error", "Este servidor solo acepta peticiones GET.");
    }

    std::string ruta = extraerRuta(peticion);
    if (ruta == "/") return menu();
    if (ruta == "/inventario") return inventario();
    if (ruta == "/servidores") return paginaServidores();
    if (ruta == "/islas") return paginaIslas();
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

// pagina de inicio del navegador: un boton por cada pagina
RespuestaHttp AtencionCliente::menu() {
    return {200, pagina_.menu((int)rutas_.activos().size(), (int)islas_.activas().size())};
}

// pagina del navegador con todos los productos: una seccion por servidor de productos de la tabla
// de rutas, con sus categorias (10) y los productos de cada una (20), y despues una seccion por
// cada otra isla activa, con sus categorias y productos pedidos con el mancomunado (10 y 20)
RespuestaHttp AtencionCliente::inventario() {
    std::vector<ServidorRuta> servidores = rutas_.activos();
    std::vector<IslaRuta> islas = islas_.activas();
    if (servidores.empty() && islas.empty()) return sinServidores();

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

    for (const auto& isla : islas) {
        BodegaVista seccion;
        seccion.id = "otra isla";
        seccion.nombre = isla.id + " (" + isla.ip + ":" + std::to_string(isla.puerto) + ")";

        std::vector<std::string> categorias;
        if (!pedirCategoriasIsla(islas_, isla, categorias)) continue;
        for (const auto& categoria : categorias) {
            CategoriaVista vista{categoria, categoria, {}};
            std::vector<ProductoDeIsla> productos;
            bool existe = true;
            if (pedirProductosIsla(islas_, isla, categoria, productos, existe)) {
                for (const auto& p : productos) vista.productos.push_back({p.nombre, p.precio, std::to_string(p.stock)});
            }
            seccion.categorias.push_back(vista);
        }
        secciones.push_back(seccion);
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

// el cliente ve una sola lista: las categorias de todos los servidores activos, sin repetir, y
// despues las de las otras islas (10 del mancomunado) que no esten ya en la isla
RespuestaHttp AtencionCliente::categorias() {
    std::vector<ServidorRuta> servidores = rutas_.activos();
    std::vector<IslaRuta> islas = islas_.activas();
    if (servidores.empty() && islas.empty()) return sinServidores();

    std::vector<CategoriaVista> todas;
    std::map<std::string, std::string> islaDe;   // categoria -> INT_NN, solo las de otras islas
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

    for (const auto& isla : islas) {
        std::vector<std::string> deIsla;
        if (!pedirCategoriasIsla(islas_, isla, deIsla)) continue;
        alguno = true;
        for (const auto& categoria : deIsla) {
            // si la isla 4 ya tiene la categoria, se queda la de la isla 4
            bool repetida = false;
            for (const auto& c : todas) {
                if (aMinusculas(c.id) == categoria) repetida = true;
            }
            if (repetida) continue;
            todas.push_back({categoria, categoria, {}});
            islaDe[categoria] = isla.id;
        }
    }
    if (!alguno) return sinComunicacion();

    std::ostringstream oss;
    oss << "<ul class=\"categorias\">\n";
    for (const auto& categoria : todas) {
        std::string isla = islaDe.count(categoria.id) ? islaDe[categoria.id] : "";
        oss << "<li data-id=\"" << pagina_.escapar(categoria.id) << "\" data-nombre=\"" << pagina_.escapar(categoria.nombre)
            << "\"" << (isla.empty() ? "" : " data-isla=\"" + isla + "\"") << "><a href=\"/productos?categoria="
            << pagina_.escapar(categoria.id) << "\">" << pagina_.escapar(categoria.nombre) << "</a>"
            << (isla.empty() ? "" : " (" + isla + ")") << "</li>\n";
    }
    oss << "</ul>\n";
    return respuesta(200, "Categorías", "ok", "", oss.str());
}

RespuestaHttp AtencionCliente::productos(const std::string& categoria) {
    // las categorias de la isla 4 son solo letras; las de otras islas siguen la regla del mancomunado
    bool esDeLaIsla = validarCategoria(categoria);
    if (!esDeLaIsla && !validarNombreMancomunado(categoria)) {
        return error(400, "categoria-invalida", "La categoría solo puede tener letras, dígitos, guion o guion bajo.");
    }

    std::vector<ServidorRuta> activos = rutas_.activos();
    std::vector<IslaRuta> islas = islas_.activas();
    if (activos.empty() && islas.empty()) return sinServidores();

    std::vector<ProductoVista> vistas;
    std::vector<std::string> islaDe;   // en paralelo con vistas: "" si el producto es de la isla 4
    bool alguno = false;

    if (esDeLaIsla && !activos.empty()) {
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
                if (!repetido) {
                    vistas.push_back(p);
                    islaDe.push_back("");
                }
            }
        };

        for (const auto& servidor : conRuta) preguntar(servidor);
        for (size_t i = 0; vistas.empty() && i < resto.size(); i++) preguntar(resto[i]);
    }

    // si la isla 4 no tiene productos de esa categoria, se le piden (20 del mancomunado)
    // a las otras islas que la tienen en su lista de categorias
    if (vistas.empty()) {
        std::string buscada = aMinusculas(categoria);
        for (const auto& isla : islas) {
            std::vector<std::string> categoriasIsla = isla.categorias;
            if (categoriasIsla.empty() && !pedirCategoriasIsla(islas_, isla, categoriasIsla)) continue;
            alguno = true;

            bool laTiene = false;
            for (const auto& c : categoriasIsla) {
                if (c == buscada) laTiene = true;
            }
            if (!laTiene) continue;

            std::vector<ProductoDeIsla> deIsla;
            bool existe = true;
            if (!pedirProductosIsla(islas_, isla, buscada, deIsla, existe)) continue;
            for (const auto& p : deIsla) {
                vistas.push_back({p.nombre, p.precio, std::to_string(p.stock)});
                islaDe.push_back(isla.id);
            }
        }
    }
    if (!alguno) return sinComunicacion();

    // 22 con count 0 en todos y ninguna isla la tiene: la categoria no existe o no tiene productos
    if (vistas.empty()) {
        return respuesta(200, "Productos", "sin-productos", "data-categoria=\"" + pagina_.escapar(categoria) + "\"",
                         "<p>No hay productos disponibles en la categoría \"" + pagina_.escapar(categoria) + "\".</p>\n");
    }

    std::ostringstream oss;
    oss << "<table class=\"productos\">\n<tr><th>Producto</th><th>Precio</th><th>Stock</th><th>Isla</th></tr>\n";
    for (size_t i = 0; i < vistas.size(); i++) {
        const auto& p = vistas[i];
        std::string isla = islaDe[i].empty() ? ID_INTERMEDIARIO : islaDe[i];
        oss << "<tr data-nombre=\"" << pagina_.escapar(p.nombre) << "\" data-precio=\"" << pagina_.escapar(p.precio)
            << "\" data-stock=\"" << pagina_.escapar(p.stock) << "\""
            << (islaDe[i].empty() ? "" : " data-isla=\"" + islaDe[i] + "\"") << "><td>" << pagina_.escapar(p.nombre)
            << "</td><td>" << pagina_.escapar(p.precio) << "</td><td>" << pagina_.escapar(p.stock) << "</td><td>"
            << isla << "</td></tr>\n";
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
    // los productos de la isla 4 no llevan guion bajo; los de otras islas pueden traerlo
    bool esDeLaIsla = validarProducto(producto);
    if (!esDeLaIsla && !validarNombreMancomunado(producto)) {
        return error(400, "producto-invalido", "El producto solo puede tener letras, dígitos, guion o guion bajo.");
    }
    if (!validarCount(cantidadTexto) || std::stoi(cantidadTexto) == 0) {
        return error(400, "cantidad-invalida", "La cantidad debe estar entre 1 y 999.");
    }

    std::vector<ServidorRuta> servidores = rutas_.activos();
    if (servidores.empty() && islas_.activas().empty()) return sinServidores();
    if (!esDeLaIsla) servidores.clear();

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
            carritos_.agregar(sesionId, {producto, reservado, textoACentavos(reserva.campos[1]), ""});
            if (primerPrecio.empty()) primerPrecio = reserva.campos[1];
            faltan -= reservado;
        }
        if (faltan > 0) {
            std::cout << "[INT] " << servidor.id << " reservo " << reservado << " de " << producto
                      << ", faltan " << faltan << "\n";
        }
    }

    // ningun servidor de la isla 4 tiene el producto: se busca en las otras islas
    if (!encontrado) {
        bool islaContesto = false;
        RespuestaHttp deIsla;
        if (agregarDeIsla(sesionId, producto, pedido, islaContesto, deIsla)) return deIsla;
        alguienContesto = alguienContesto || islaContesto;
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

// busca el producto en lo que las otras islas ya mostraron con un 20. si ninguna lo ha mostrado,
// se les piden los productos de todas sus categorias. luego se aparta con el 50 del mancomunado
// en la primera isla que tenga stock; como el 50 es todo o nada, se pide lo que esa isla tiene si
// es menos de lo pedido. devuelve true si ya armo la respuesta (agregado o sin stock)
bool AtencionCliente::agregarDeIsla(int sesionId, const std::string& producto, int pedido, bool& contesto,
                                    RespuestaHttp& resultado) {
    contesto = false;
    std::vector<ProductoDeIsla> candidatos = islas_.buscarProducto(producto);
    if (candidatos.empty()) {
        for (const auto& isla : islas_.activas()) {
            std::vector<std::string> categorias = isla.categorias;
            if (categorias.empty() && !pedirCategoriasIsla(islas_, isla, categorias)) continue;
            for (const auto& categoria : categorias) {
                std::vector<ProductoDeIsla> productos;
                bool existe = true;
                pedirProductosIsla(islas_, isla, categoria, productos, existe);
            }
        }
        candidatos = islas_.buscarProducto(producto);
    }
    if (candidatos.empty()) return false;
    contesto = true;

    bool habiaPeroSinStock = false;
    for (const auto& candidato : candidatos) {
        int cantidad = std::min(pedido, candidato.stock);
        if (cantidad == 0) {
            habiaPeroSinStock = true;
            continue;
        }

        IslaRuta isla;
        bool activa = false;
        for (const auto& i : islas_.activas()) {
            if (i.id == candidato.isla) {
                isla = i;
                activa = true;
            }
        }
        if (!activa) continue;

        MensajeMancomunado respuestaIsla;
        if (!consultarIsla(islas_, isla, MAN_RESERVE, {candidato.nombre, std::to_string(cantidad)}, respuestaIsla)) {
            continue;
        }
        if (respuestaIsla.tipo == MAN_ERROR && !respuestaIsla.campos.empty() &&
            respuestaIsla.campos[0] == std::to_string(ERR_STOCK)) {
            habiaPeroSinStock = true;
            continue;
        }
        if (respuestaIsla.tipo != MAN_ACK) continue;   // 03 u otro error: se prueba la siguiente isla

        carritos_.agregar(sesionId, {candidato.nombre, cantidad, textoACentavos(candidato.precio), isla.id});
        std::cout << "[INT] " << cantidad << " x " << candidato.nombre << " reservado en " << isla.id
                  << " por " << Reservas::SEGUNDOS_VIGENCIA << " s\n";
        std::string atributos = "data-producto=\"" + pagina_.escapar(candidato.nombre) + "\" data-precio=\"" +
                                candidato.precio + "\" data-cantidad=\"" + std::to_string(cantidad) +
                                "\" data-pedido=\"" + std::to_string(pedido) + "\" data-isla=\"" + isla.id + "\"";
        resultado = respuesta(200, "Agregar", "agregado", atributos,
                              "<p>Agregado " + std::to_string(cantidad) + " x " + pagina_.escapar(candidato.nombre) +
                              " @ " + candidato.precio + " de " + isla.id + "</p>\n");
        return true;
    }

    if (habiaPeroSinStock) {
        resultado = respuesta(200, "Agregar", "sin-stock", "data-producto=\"" + pagina_.escapar(producto) + "\"",
                              "<p>No hay stock de " + pagina_.escapar(producto) + ".</p>\n");
        return true;
    }
    return false;
}

// compra en otra isla (40 del mancomunado) lo que el carrito tiene reservado alla. si la reserva
// vencio (02/104), se vuelve a reservar con 50 y se intenta el 40 una vez mas. deja en subtotales
// lo que la otra isla cobro por cada producto; devuelve false si no se pudo comprar
bool AtencionCliente::comprarEnIsla(const std::string& idIsla, const std::vector<ItemReservado>& items,
                                    std::map<std::string, uint64_t>& subtotales) {
    IslaRuta isla;
    bool activa = false;
    for (const auto& i : islas_.activas()) {
        if (i.id == idIsla) {
            isla = i;
            activa = true;
        }
    }
    if (!activa) return false;

    std::string lista;
    for (const auto& item : items) {
        lista += (lista.empty() ? "" : ";") + item.producto + "," + std::to_string(item.cantidad);
    }

    for (int intento = 1; intento <= 2; intento++) {
        MensajeMancomunado respuestaIsla;
        if (!consultarIsla(islas_, isla, MAN_BUY, {std::to_string(items.size()), lista}, respuestaIsla)) return false;

        if (respuestaIsla.tipo == MAN_ACK && respuestaIsla.campos.size() >= 3) {
            // 01/total/n/producto,cantidad,subtotal;...
            for (const auto& linea : separar(respuestaIsla.campos[2], ';')) {
                std::vector<std::string> partes = separar(linea, ',');
                if (partes.size() == 3 && validarPrecio(partes[2])) {
                    subtotales[aMinusculas(partes[0])] = textoACentavos(partes[2]);
                }
            }
            return true;
        }

        bool vencida = respuestaIsla.tipo == MAN_ERROR && !respuestaIsla.campos.empty() &&
                       respuestaIsla.campos[0] == std::to_string(ERR_SIN_RESERVA);
        if (!vencida || intento == 2) return false;

        std::cout << "[INT] la reserva en " << idIsla << " vencio, se vuelve a reservar\n";
        for (const auto& item : items) {
            MensajeMancomunado reserva;
            if (!consultarIsla(islas_, isla, MAN_RESERVE, {item.producto, std::to_string(item.cantidad)}, reserva) ||
                reserva.tipo != MAN_ACK) {
                return false;
            }
        }
    }
    return false;
}

// arma la factura con lo ya reservado en el carrito. lo de la isla 4 no se vuelve a consultar;
// lo de otras islas se compra con un 40 a cada isla, y lo que no se pudo comprar sale sin cobrar
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

    // una compra va a una sola isla y genera una factura por isla: se agrupan por isla
    std::map<std::string, std::vector<ItemReservado>> porIsla;
    for (const auto& item : items) {
        if (!item.isla.empty()) porIsla[item.isla].push_back(item);
    }
    std::map<std::string, bool> comprado;
    std::map<std::string, uint64_t> subtotalesIsla;   // "INT_NN/producto en minusculas" -> lo que cobro
    for (const auto& par : porIsla) {
        std::map<std::string, uint64_t> subtotales;
        comprado[par.first] = comprarEnIsla(par.first, par.second, subtotales);
        for (const auto& s : subtotales) subtotalesIsla[par.first + "/" + s.first] = s.second;
    }

    uint64_t total = 0;
    std::string listaProtocolo;
    std::ostringstream filas;
    filas << "<table class=\"factura\">\n<tr><th>Producto</th><th>Cantidad</th><th>Precio</th><th>Subtotal</th>"
          << "<th>Isla</th><th>Estado</th></tr>\n";

    for (const auto& item : items) {
        bool cobrado = item.isla.empty() || comprado[item.isla];
        uint64_t subtotal = (uint64_t)item.precioCentavos * item.cantidad;
        std::string llave = item.isla + "/" + aMinusculas(item.producto);
        if (!item.isla.empty() && subtotalesIsla.count(llave)) subtotal = subtotalesIsla[llave];
        if (cobrado) total += subtotal;

        std::string isla = item.isla.empty() ? ID_INTERMEDIARIO : item.isla;
        std::string estado = cobrado ? "ok" : "no-comprado";
        filas << "<tr data-producto=\"" << pagina_.escapar(item.producto) << "\" data-cantidad=\"" << item.cantidad
              << "\" data-subtotal=\"" << centavosATexto(cobrado ? subtotal : 0) << "\" data-isla=\"" << isla
              << "\" data-estado=\"" << estado << "\"><td>" << pagina_.escapar(item.producto)
              << "</td><td>" << item.cantidad << "</td><td>" << centavosATexto(item.precioCentavos)
              << "</td><td>" << centavosATexto(cobrado ? subtotal : 0) << "</td><td>" << isla << "</td><td>"
              << (cobrado ? "" : "no se pudo comprar") << "</td></tr>\n";

        if (!cobrado) continue;
        if (!listaProtocolo.empty()) listaProtocolo += ";";
        listaProtocolo += item.producto + "," + std::to_string(item.cantidad) + "," + centavosATexto(subtotal);
    }
    filas << "</table>\n<p>Total: " << centavosATexto(total) << "</p>\n";

    // el registro usa el mismo formato del mensaje 41: total, cantidad de productos y detalle
    std::cout << "[INT] factura sesion " << sesionId << ": 41/" << centavosATexto(total) << "/"
              << items.size() << "/" << listaProtocolo << "\n";

    return respuesta(200, "Factura proforma", "ok", "data-total=\"" + centavosATexto(total) + "\"", filas.str());
}

// tabla de los servidores de productos descubiertos, con su estado
std::string AtencionCliente::tablaServidores(const std::vector<ServidorRuta>& servidores, bool conTitulo) {
    std::ostringstream oss;
    if (conTitulo) oss << "<h2>Servidores de productos</h2>\n";
    oss << "<table class=\"servidores\">\n"
        << "<tr><th>Servidor</th><th>Dirección</th><th>Estado</th><th>Último contacto</th><th>Categorías</th></tr>\n";
    for (const auto& s : servidores) {
        oss << "<tr data-id=\"" << s.id << "\" data-estado=\"" << TablaRutas::estado(s) << "\"><td>" << s.id
            << "</td><td>" << s.ip << ":" << s.puerto << "</td><td>" << TablaRutas::estado(s) << "</td><td>hace "
            << (time(nullptr) - s.ultimoContacto) << " s</td><td>" << s.categorias.size() << "</td></tr>\n";
    }
    oss << "</table>\n";
    if (servidores.empty()) oss << "<p>Todavía no se ha descubierto ningún servidor de productos.</p>\n";
    return oss.str();
}

// una fila por cada categoria que anuncio cada servidor; filas queda con cuantas son
std::string AtencionCliente::tablaRutasCategorias(const std::vector<ServidorRuta>& servidores, int& filas) {
    std::ostringstream oss;
    oss << "<h2>Rutas por categoría</h2>\n<table class=\"rutas\">\n"
        << "<tr><th>Categoría</th><th>Servidor</th><th>Dirección</th><th>Estado</th></tr>\n";
    filas = 0;
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
    return oss.str();
}

// otras islas descubiertas con el 60 del mancomunado
std::string AtencionCliente::tablaIslas(const std::vector<IslaRuta>& islas, bool conTitulo) {
    std::ostringstream oss;
    if (conTitulo) oss << "<h2>Otras islas</h2>\n";
    oss << "<table class=\"islas\">\n"
        << "<tr><th>Isla</th><th>Dirección</th><th>Estado</th><th>Último anuncio</th><th>Categorías</th></tr>\n";
    for (const auto& isla : islas) {
        std::string estado = TablaIslas::estado(isla);
        std::string categorias;
        for (const auto& c : isla.categorias) categorias += (categorias.empty() ? "" : ", ") + c;
        oss << "<tr data-isla=\"" << isla.id << "\" data-estado=\"" << estado << "\"><td>" << isla.id << "</td><td>"
            << isla.ip << ":" << isla.puerto << "</td><td>" << estado << "</td><td>hace "
            << (time(nullptr) - isla.ultimoAnuncio) << " s</td><td>" << pagina_.escapar(categorias) << "</td></tr>\n";
    }
    oss << "</table>\n";
    if (islas.empty()) oss << "<p>Todavía no se ha descubierto ninguna otra isla.</p>\n";
    return oss.str();
}

// tabla de rutas completa: servidores, rutas por categoria y otras islas. la usa el comando RUTAS
RespuestaHttp AtencionCliente::paginaRutas() {
    std::vector<ServidorRuta> servidores = rutas_.todos();
    std::vector<IslaRuta> islas = islas_.todas();
    int filas = 0;

    std::ostringstream oss;
    oss << "<meta http-equiv=\"refresh\" content=\"2\">\n" << tablaServidores(servidores, true)
        << tablaRutasCategorias(servidores, filas) << tablaIslas(islas, true);

    return respuesta(200, "Tabla de rutas", "ok",
                     "data-servidores=\"" + std::to_string(servidores.size()) + "\" data-rutas=\"" +
                     std::to_string(filas) + "\" data-islas=\"" + std::to_string(islas.size()) + "\"", oss.str());
}

RespuestaHttp AtencionCliente::paginaServidores() {
    std::vector<ServidorRuta> servidores = rutas_.todos();
    return respuesta(200, "Servidores", "ok", "data-servidores=\"" + std::to_string(servidores.size()) + "\"",
                     "<meta http-equiv=\"refresh\" content=\"2\">\n" + tablaServidores(servidores, false));
}

RespuestaHttp AtencionCliente::paginaIslas() {
    std::vector<IslaRuta> islas = islas_.todas();
    return respuesta(200, "Otras islas", "ok", "data-islas=\"" + std::to_string(islas.size()) + "\"",
                     "<meta http-equiv=\"refresh\" content=\"2\">\n" + tablaIslas(islas, false));
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
    return consultarServidor(rutas_, servidor, tipo, campos, respuestaMsg);
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