#include "AtencionBodega.h"
#include "Validaciones.h"
#include <cstring>
#include <stdexcept>

static std::string textoCampo(const char* campo, size_t tam) {
    return std::string(campo, strnlen(campo, tam));
}

// un valor recibido con / o | romperia la respuesta de error que lo devuelve
static std::string limpiarValor(std::string valor) {
    for (char& c : valor) {
        if (c == '/' || c == '|') c = '_';
    }
    return valor;
}

AtencionBodega::AtencionBodega(AdministradorBodegas& admin) : admin_(admin) {}

std::string AtencionBodega::responder(const std::string& texto) {
    if (!validarTamanoMensaje(texto)) {
        return construirMensaje(ID_SERVIDOR, ID_INTERMEDIARIO, TIPO_ERR_TAMANO,
            {"00", "mensaje", std::to_string(texto.size()), std::to_string(TAM_MAX_MENSAJE)});
    }

    Mensaje msg;
    try {
        msg = parsearMensaje(texto);
    } catch (const std::exception&) {
        return construirMensaje(ID_SERVIDOR, ID_INTERMEDIARIO, TIPO_ERR_FORMATO, {"00", "mensaje", "invalido"});
    }

    // la respuesta va a quien envio el mensaje, si su id es valido
    std::string destino = validarOrigenDestino(msg.origen) ? msg.origen : ID_INTERMEDIARIO;
    std::string tipoOriginal = std::to_string(msg.tipo);

    // solo un intermediario puede hablar con el servidor de productos
    if (!validarOrigenDestino(msg.origen) || msg.origen.rfind("INT_", 0) != 0) {
        return errorFormato(destino, tipoOriginal, "origen", msg.origen);
    }
    if (msg.destino != ID_SERVIDOR) {
        return errorFormato(destino, tipoOriginal, "destino", msg.destino);
    }

    std::string respuesta;
    switch (msg.tipo) {
        case TIPO_REQUEST_CATEGORIES:
            respuesta = listaCategorias(destino);
            break;
        case TIPO_REQUEST_PRODUCTS:
            respuesta = listaProductos(msg, destino);
            break;
        case TIPO_RESERVE_STOCK:
            respuesta = reservar(msg, destino);
            break;
        default:
            return errorFormato(destino, tipoOriginal, "tipo_mensaje", tipoOriginal);
    }

    // una lista muy larga puede pasar del maximo de TAM_MAX_MENSAJE
    if (!validarTamanoMensaje(respuesta)) {
        return construirMensaje(ID_SERVIDOR, destino, TIPO_ERR_TAMANO,
            {tipoOriginal, "respuesta", std::to_string(respuesta.size()), std::to_string(TAM_MAX_MENSAJE)});
    }
    return respuesta;
}

std::string AtencionBodega::listaCategorias(const std::string& destino) {
    std::string lista;
    for (const auto& id : admin_.listarIdsCategorias()) {
        if (!lista.empty()) lista += ",";
        lista += id;
    }
    return construirMensaje(ID_SERVIDOR, destino, TIPO_CATEGORY_LIST, {lista});
}

std::string AtencionBodega::listaProductos(const Mensaje& msg, const std::string& destino) {
    if (msg.campos.empty() || !validarCategoria(msg.campos[0])) {
        return errorFormato(destino, std::to_string(msg.tipo), "categoria", msg.campos.empty() ? "" : msg.campos[0]);
    }

    // una categoria que no existe se responde igual que una vacia: 22/0
    std::vector<RegistroProducto> productos;
    admin_.productosDeCategoria(msg.campos[0], productos);

    if (productos.empty()) {
        return construirMensaje(ID_SERVIDOR, destino, TIPO_PRODUCT_LIST, {"0"});
    }

    // lista con el formato nombre,precio,stock;nombre,precio,stock
    std::string lista;
    for (const auto& p : productos) {
        if (!lista.empty()) lista += ";";
        lista += textoCampo(p.nombre, sizeof(p.nombre)) + "," + precioATexto(p.precioCentavos) + "," +
                 std::to_string(p.cantidad);
    }
    return construirMensaje(ID_SERVIDOR, destino, TIPO_PRODUCT_LIST, {std::to_string(productos.size()), lista});
}

std::string AtencionBodega::reservar(const Mensaje& msg, const std::string& destino) {
    std::string tipoOriginal = std::to_string(msg.tipo);
    if (msg.campos.size() < 2 || !validarProducto(msg.campos[0])) {
        return errorFormato(destino, tipoOriginal, "producto", msg.campos.empty() ? "" : msg.campos[0]);
    }
    if (!validarCount(msg.campos[1])) {
        return errorFormato(destino, tipoOriginal, "cantidad", msg.campos[1]);
    }

    int reservado = 0;
    uint32_t precioCentavos = 0;
    if (!admin_.reservar(msg.campos[0], std::stoi(msg.campos[1]), reservado, precioCentavos)) {
        return construirMensaje(ID_SERVIDOR, destino, TIPO_PRODUCT_NOT_FOUND, {msg.campos[0]});
    }

    // reservado puede ser menor al pedido, o 0, si no habia suficiente stock
    return construirMensaje(ID_SERVIDOR, destino, TIPO_RESERVED,
                            {msg.campos[0], precioATexto(precioCentavos), std::to_string(reservado)});
}

// mensaje 90: tipo original, campo invalido y valor recibido
std::string AtencionBodega::errorFormato(const std::string& destino, const std::string& tipoOriginal,
                                         const std::string& campo, const std::string& valor) {
    return construirMensaje(ID_SERVIDOR, destino, TIPO_ERR_FORMATO, {tipoOriginal, campo, limpiarValor(valor)});
}