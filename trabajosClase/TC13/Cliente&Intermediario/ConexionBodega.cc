#include "ConexionBodega.h"
#include "Socket.h"
#include "Protocolo.h"
#include <chrono>
#include <thread>
#include <stdexcept>

static const int REINTENTOS = 2;           // intentos extra despues del primero
static const int SEGUNDOS_ESPERA = 3;      // tiempo maximo de connect y de read
static const int MILISEGUNDOS_ENTRE_INTENTOS = 500;

ConexionBodega::ConexionBodega(const std::string& ip, int puerto) : ip_(ip), puerto_(puerto) {}

bool ConexionBodega::enviar(const std::string& mensaje, std::string& respuesta, int& reintentos) {
    for (reintentos = 0; reintentos <= REINTENTOS; reintentos++) {
        if (reintentos > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(MILISEGUNDOS_ENTRE_INTENTOS));
        }
        if (intentarUnaVez(mensaje, respuesta)) return true;
    }
    reintentos = REINTENTOS;
    return false;
}

bool ConexionBodega::intentarUnaVez(const std::string& mensaje, std::string& respuesta) {
    try {
        Socket conexion('s');
        conexion.SetTimeout(SEGUNDOS_ESPERA);
        conexion.Connect(ip_.c_str(), puerto_);

        std::string linea = mensaje + "\n";
        conexion.Write(linea.data(), linea.size());

        // se lee hasta el '\n' que cierra la respuesta o hasta que el servidor cierre la conexion
        char buf[512];
        std::string acumulado;
        while (acumulado.find('\n') == std::string::npos && acumulado.size() <= 2 * TAM_MAX_MENSAJE) {
            size_t leidos = conexion.Read(buf, sizeof(buf));
            if (leidos == 0) break;
            acumulado.append(buf, leidos);
        }

        size_t fin = acumulado.find('\n');
        if (fin != std::string::npos) acumulado.erase(fin);
        if (!acumulado.empty() && acumulado.back() == '\r') acumulado.pop_back();
        if (acumulado.empty()) return false;

        respuesta = acumulado;
        return true;

    } catch (const std::exception&) {
        // connect rechazado, tiempo de espera agotado o conexion cortada: cuenta como intento fallido
        return false;
    }
}