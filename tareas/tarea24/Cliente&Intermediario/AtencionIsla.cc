#include "AtencionIsla.h"
#include "ConsultaServidor.h"
#include "Protocolo.h"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <stdexcept>

// el protocolo interno manda cantidades de hasta 3 digitos (count), el mancomunado de hasta 5;
// una cantidad mayor se pide al servidor en varias partes
static const int MAX_CANTIDAD_INTERNA = 999;

static std::vector<std::string> separar(const std::string& texto, char separador) {
    std::vector<std::string> partes;
    std::stringstream ss(texto);
    std::string parte;
    while (std::getline(ss, parte, separador)) partes.push_back(parte);
    return partes;
}

// "1100.00" -> 110000
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

// el total de una compra puede pasar de uint32_t
static std::string centavosATexto(uint64_t centavos) {
    char texto[32];
    snprintf(texto, sizeof(texto), "%llu.%02llu", (unsigned long long)(centavos / 100),
             (unsigned long long)(centavos % 100));
    return texto;
}

AtencionIsla::AtencionIsla(TablaRutas& rutas, Reservas& reservas) : rutas_(rutas), reservas_(reservas) {}

std::string AtencionIsla::error(int codigo) {
    return armarMancomunado(MAN_ERROR, {std::to_string(codigo)});
}

std::string AtencionIsla::noEncontrado(const std::string& valor) {
    return armarMancomunado(MAN_NOT_FOUND, {valor});
}

std::string AtencionIsla::responder(const std::string& linea, const std::string& ipIsla) {
    MensajeMancomunado msg;
    try {
        msg = parsearMancomunado(linea);
    } catch (const std::exception&) {
        return error(ERR_MAL_FORMADO);
    }

    switch (msg.tipo) {
        case MAN_GET_CATEGORIES:
            return msg.campos.empty() ? categorias() : error(ERR_MAL_FORMADO);
        case MAN_GET_PRODUCTS:
            return productos(msg);
        case MAN_RESERVE:
            return reservar(msg, ipIsla);
        case MAN_BUY:
            return comprar(msg, ipIsla);
        default:
            // 60 y 61 van por udp; 01, 02 y 03 son respuestas, no solicitudes
            return error(ERR_TIPO_NO_SOPORTADO);
    }
}

// 10 -> 01/cat1,cat2,... con las categorias de todos los servidores activos, en minusculas y sin repetir.
// salen de la tabla de rutas, que se actualiza con cada 83
std::string AtencionIsla::categorias() {
    std::vector<std::string> lista;
    for (const auto& servidor : rutas_.activos()) {
        for (const auto& categoria : servidor.categorias) {
            std::string minuscula = aMinusculas(categoria);
            bool repetida = false;
            for (const auto& c : lista) {
                if (c == minuscula) repetida = true;
            }
            if (!repetida) lista.push_back(minuscula);
        }
    }

    std::string texto;
    for (const auto& c : lista) texto += (texto.empty() ? "" : ",") + c;
    return armarMancomunado(MAN_ACK, {texto});
}

bool AtencionIsla::buscarCategoria(const std::string& categoria, std::string& nombreReal, ServidorRuta& servidor) {
    for (const auto& s : rutas_.activos()) {
        for (const auto& c : s.categorias) {
            if (aMinusculas(c) != categoria) continue;
            // la ruta de la categoria es del servidor que la anuncio primero
            std::vector<ServidorRuta> dueno = rutas_.conCategoria(c);
            if (dueno.empty()) continue;
            nombreReal = c;
            servidor = dueno[0];
            return true;
        }
    }
    return false;
}

bool AtencionIsla::productosDe(const ServidorRuta& servidor, const std::string& categoria, std::string& count,
                               std::string& lista) {
    Mensaje respuesta;
    if (!consultarServidor(rutas_, servidor, TIPO_REQUEST_PRODUCTS, {categoria}, respuesta)) return false;
    if (respuesta.tipo != TIPO_PRODUCT_LIST || respuesta.campos.empty()) return false;
    count = respuesta.campos[0];
    lista = respuesta.campos.size() >= 2 ? respuesta.campos[1] : "";
    return true;
}

// 20/categoria -> 01/n/producto,precio,stock;...  o 03/categoria si la isla no la tiene.
// la lista del protocolo interno ya viene como producto,precio,stock, asi que pasa tal cual
std::string AtencionIsla::productos(const MensajeMancomunado& msg) {
    if (msg.campos.size() != 1) return error(ERR_MAL_FORMADO);
    if (!validarNombreMancomunado(msg.campos[0])) return error(ERR_VALOR_INVALIDO);

    std::string categoria = aMinusculas(msg.campos[0]);
    std::string nombreReal;
    ServidorRuta servidor;
    if (!buscarCategoria(categoria, nombreReal, servidor)) return noEncontrado(categoria);

    std::string count;
    std::string lista;
    if (!productosDe(servidor, nombreReal, count, lista)) {
        // el servidor dueno no contesto: para la otra isla la categoria no esta disponible ahora
        return noEncontrado(categoria);
    }
    if (count == "0") return armarMancomunado(MAN_ACK, {"0"});
    return armarMancomunado(MAN_ACK, {count, lista});
}

bool AtencionIsla::buscarProducto(const std::string& producto, int pedido, const std::string& soloServidor,
                                  ProductoIsla& encontrado) {
    bool hay = false;
    for (const auto& servidor : rutas_.activos()) {
        if (!soloServidor.empty() && servidor.id != soloServidor) continue;
        for (const auto& categoria : servidor.categorias) {
            std::string count;
            std::string lista;
            if (!productosDe(servidor, categoria, count, lista) || count == "0") continue;

            for (const auto& item : separar(lista, ';')) {
                std::vector<std::string> partes = separar(item, ',');
                if (partes.size() != 3 || aMinusculas(partes[0]) != producto) continue;
                int stock = std::stoi(partes[2]);
                if (!hay || stock > encontrado.stock) encontrado = {servidor, partes[0], stock};
                hay = true;
                if (stock >= pedido) return true;
            }
        }
    }
    return hay;
}

// 50/producto/cantidad -> 01 si se aparto todo, 02/103 si no alcanza, 03/producto si no existe.
// el stock se descuenta en el servidor con el mensaje interno 50 y queda anotado en reservas_
std::string AtencionIsla::reservar(const MensajeMancomunado& msg, const std::string& ipIsla) {
    if (msg.campos.size() != 2) return error(ERR_MAL_FORMADO);
    if (!validarNombreMancomunado(msg.campos[0]) || !validarCantidadMancomunado(msg.campos[1])) {
        return error(ERR_VALOR_INVALIDO);
    }
    int pedido = std::stoi(msg.campos[1]);
    if (pedido == 0) return error(ERR_VALOR_INVALIDO);

    // si esa isla ya tiene una reserva del producto, se sigue usando el mismo servidor
    std::string producto = aMinusculas(msg.campos[0]);
    ProductoIsla encontrado;
    if (!buscarProducto(producto, pedido, reservas_.servidorDe(ipIsla, producto), encontrado)) {
        return noEncontrado(msg.campos[0]);
    }
    if (encontrado.stock < pedido) {
        std::cout << "[INT] " << encontrado.nombre << " en " << encontrado.servidor.id << " tiene "
                  << encontrado.stock << ", no alcanza para " << pedido << "\n";
        return error(ERR_STOCK);
    }

    // se pide en partes de hasta 999, que es lo que acepta el protocolo interno
    int reservado = 0;
    uint32_t precio = 0;
    while (reservado < pedido) {
        int parte = std::min(pedido - reservado, MAX_CANTIDAD_INTERNA);
        Mensaje respuesta;
        if (!consultarServidor(rutas_, encontrado.servidor, TIPO_RESERVE_STOCK,
                               {encontrado.nombre, std::to_string(parte)}, respuesta) ||
            respuesta.tipo != TIPO_RESERVED || respuesta.campos.size() < 3) {
            break;
        }
        int esta = std::stoi(respuesta.campos[2]);
        precio = textoACentavos(respuesta.campos[1]);
        reservado += esta;
        if (esta < parte) break;
    }

    // entre la consulta y la reserva otro cliente pudo llevarse stock: lo que si se aparto se devuelve
    if (reservado < pedido) {
        if (reservado > 0) devolverReserva(rutas_, encontrado.servidor.id, encontrado.nombre, reservado);
        return error(ERR_STOCK);
    }

    reservas_.agregar(ipIsla, encontrado.nombre, encontrado.servidor.id, pedido, precio);
    std::cout << "[INT] reserva de " << ipIsla << ": " << pedido << " x " << encontrado.nombre << " en "
              << encontrado.servidor.id << " por " << Reservas::SEGUNDOS_VIGENCIA << " s\n";
    return armarMancomunado(MAN_ACK, {});
}

// 40/n/producto,cantidad;... -> 01/total/n/producto,cantidad,subtotal;...
// cada producto tiene que tener una reserva vigente de esa misma isla; si uno no la tiene, 02/104
std::string AtencionIsla::comprar(const MensajeMancomunado& msg, const std::string& ipIsla) {
    if (msg.campos.size() != 2 || !validarCountMancomunado(msg.campos[0])) return error(ERR_MAL_FORMADO);
    int n = std::stoi(msg.campos[0]);

    std::vector<ItemCompra> items;
    for (const auto& item : separar(msg.campos[1], ';')) {
        std::vector<std::string> partes = separar(item, ',');
        if (partes.size() != 2) return error(ERR_MAL_FORMADO);
        if (!validarNombreMancomunado(partes[0]) || !validarCantidadMancomunado(partes[1]) || std::stoi(partes[1]) == 0) {
            return error(ERR_VALOR_INVALIDO);
        }
        items.push_back({partes[0], std::stoi(partes[1])});
    }
    if (n == 0 || (int)items.size() != n) return error(ERR_VALOR_INVALIDO);

    std::vector<Reserva> comprados;
    std::string faltante;
    if (!reservas_.comprar(ipIsla, items, comprados, faltante)) {
        std::cout << "[INT] " << ipIsla << " no tiene reserva vigente que alcance para " << faltante << "\n";
        return error(ERR_SIN_RESERVA);
    }

    uint64_t total = 0;
    std::string detalle;
    for (const auto& c : comprados) {
        uint64_t subtotal = (uint64_t)c.precioCentavos * c.cantidad;
        total += subtotal;
        detalle += (detalle.empty() ? "" : ";") + c.producto + "," + std::to_string(c.cantidad) + "," +
                   centavosATexto(subtotal);
    }
    std::cout << "[INT] compra de " << ipIsla << " por " << centavosATexto(total) << "\n";
    return armarMancomunado(MAN_ACK, {centavosATexto(total), std::to_string(comprados.size()), detalle});
}

void devolverReserva(TablaRutas& rutas, const std::string& idServidor, const std::string& producto, int cantidad) {
    ServidorRuta servidor;
    bool existe = false;
    for (const auto& s : rutas.todos()) {
        if (s.id == idServidor) {
            servidor = s;
            existe = true;
        }
    }
    if (!existe) {
        std::cout << "[INT] " << idServidor << " ya no esta en la tabla, no se pueden devolver " << cantidad
                  << " x " << producto << "\n";
        return;
    }

    for (int devuelto = 0; devuelto < cantidad;) {
        int parte = std::min(cantidad - devuelto, MAX_CANTIDAD_INTERNA);
        Mensaje respuesta;
        if (!consultarServidor(rutas, servidor, TIPO_RELEASE_STOCK, {producto, std::to_string(parte)}, respuesta)) {
            return;
        }
        devuelto += parte;
    }
}
