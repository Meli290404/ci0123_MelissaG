#include "ConexionIsla.h"
#include "Socket.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <sstream>
#include <cctype>

static const int MILISEGUNDOS_ENTRE_INTENTOS = 500;
static const size_t MAX_RESPUESTA_ISLA = 65536;   // el mancomunado no tiene maximo; esto evita llenar la memoria

ConexionIsla::ConexionIsla(const std::string& ip, int puerto) : ip_(ip), puerto_(puerto) {}

bool ConexionIsla::enviar(const std::string& mensaje, std::string& respuesta, int& intentos) {
    for (intentos = 1; intentos <= INTENTOS; intentos++) {
        if (intentos > 1) std::this_thread::sleep_for(std::chrono::milliseconds(MILISEGUNDOS_ENTRE_INTENTOS));
        if (intentarUnaVez(mensaje, respuesta)) return true;
    }
    intentos = INTENTOS;
    return false;
}

bool ConexionIsla::intentarUnaVez(const std::string& mensaje, std::string& respuesta) {
    std::string acumulado;
    try {
        Socket conexion('s');
        conexion.SetTimeout(SEGUNDOS_ESPERA);
        conexion.Connect(ip_.c_str(), puerto_);

        std::string linea = mensaje + "\n";
        conexion.Write(linea.data(), linea.size());

        // se lee hasta el '\n' o hasta que la otra isla cierre la conexion
        char buf[1024];
        while (acumulado.find('\n') == std::string::npos && acumulado.size() <= MAX_RESPUESTA_ISLA) {
            size_t leidos = conexion.Read(buf, sizeof(buf));
            if (leidos == 0) break;
            acumulado.append(buf, leidos);
        }
    } catch (const std::exception&) {
        // connect rechazado o tiempo agotado. si ya habia llegado algo sin '\n', se toma como la respuesta
        if (acumulado.empty()) return false;
    }

    size_t fin = acumulado.find('\n');
    if (fin != std::string::npos) acumulado.erase(fin);
    if (!acumulado.empty() && acumulado.back() == '\r') acumulado.pop_back();
    if (acumulado.empty()) return false;

    respuesta = acumulado;
    return true;
}

bool consultarIsla(TablaIslas& islas, const IslaRuta& isla, int tipo, const std::vector<std::string>& campos,
                   MensajeMancomunado& respuesta) {
    std::string enviado = armarMancomunado(tipo, campos);
    std::cout << "[MAN TCP INT -> " << isla.id << "] " << enviado << "\n";

    std::string texto;
    int intentos = 0;
    ConexionIsla conexion(isla.ip, isla.puerto);
    if (!conexion.enviar(enviado, texto, intentos)) {
        std::cout << "[INT] " << isla.id << " no contesto despues de " << intentos
                  << " intentos, se deshabilita en la tabla de islas hasta su proximo 60\n";
        islas.marcarSinRespuesta(isla.id);
        return false;
    }
    std::cout << "[MAN TCP " << isla.id << " -> INT] " << texto << "\n";

    try {
        respuesta = parsearMancomunado(texto);
    } catch (const std::exception&) {
        std::cout << "[INT] " << isla.id << " respondio algo que no cumple el protocolo\n";
        return false;
    }
    if (respuesta.tipo != MAN_ACK && respuesta.tipo != MAN_ERROR && respuesta.tipo != MAN_NOT_FOUND) {
        std::cout << "[INT] " << isla.id << " respondio un tipo " << respuesta.tipo << ", que no es respuesta\n";
        return false;
    }
    return true;
}

static std::vector<std::string> separar(const std::string& texto, char separador) {
    std::vector<std::string> partes;
    std::stringstream ss(texto);
    std::string parte;
    while (std::getline(ss, parte, separador)) partes.push_back(parte);
    return partes;
}

// precio del mancomunado: ^[0-9]{1,6}(\.[0-9]{1,2})?$
static bool validarPrecioMancomunado(const std::string& s) {
    size_t punto = s.find('.');
    std::string entera = s.substr(0, punto);
    std::string decimal = (punto == std::string::npos) ? "" : s.substr(punto + 1);
    if (entera.empty() || entera.size() > 6) return false;
    if (punto != std::string::npos && (decimal.empty() || decimal.size() > 2)) return false;
    for (char c : entera + decimal) {
        if (!isdigit((unsigned char)c)) return false;
    }
    return true;
}

bool pedirCategoriasIsla(TablaIslas& islas, const IslaRuta& isla, std::vector<std::string>& categorias) {
    MensajeMancomunado respuesta;
    if (!consultarIsla(islas, isla, MAN_GET_CATEGORIES, {}, respuesta) || respuesta.tipo != MAN_ACK) return false;

    // 01/cat1,cat2,... ; una isla sin categorias puede contestar solo 01
    categorias.clear();
    if (!respuesta.campos.empty()) {
        for (const auto& c : separar(respuesta.campos[0], ',')) {
            if (!validarNombreMancomunado(c)) {
                std::cout << "[INT] " << isla.id << " mando una categoria invalida: " << c << "\n";
                return false;
            }
            categorias.push_back(aMinusculas(c));
        }
    }
    islas.guardarCategorias(isla.id, categorias);
    return true;
}

bool pedirProductosIsla(TablaIslas& islas, const IslaRuta& isla, const std::string& categoria,
                        std::vector<ProductoDeIsla>& productos, bool& existe) {
    productos.clear();
    existe = true;
    MensajeMancomunado respuesta;
    if (!consultarIsla(islas, isla, MAN_GET_PRODUCTS, {categoria}, respuesta)) return false;
    if (respuesta.tipo == MAN_NOT_FOUND) {
        existe = false;
        return true;
    }
    if (respuesta.tipo != MAN_ACK || respuesta.campos.empty() || !validarCountMancomunado(respuesta.campos[0])) {
        return false;
    }

    // 01/cantidad/producto,precio,stock;...
    int cantidad = std::stoi(respuesta.campos[0]);
    if (cantidad > 0) {
        if (respuesta.campos.size() < 2) return false;
        for (const auto& item : separar(respuesta.campos[1], ';')) {
            std::vector<std::string> partes = separar(item, ',');
            if (partes.size() != 3 || !validarNombreMancomunado(partes[0]) || !validarPrecioMancomunado(partes[1]) ||
                !validarCantidadMancomunado(partes[2])) {
                std::cout << "[INT] " << isla.id << " mando un producto invalido: " << item << "\n";
                return false;
            }
            productos.push_back({isla.id, partes[0], partes[1], std::stoi(partes[2])});
        }
    }
    islas.guardarProductos(isla.id, productos);
    return true;
}
