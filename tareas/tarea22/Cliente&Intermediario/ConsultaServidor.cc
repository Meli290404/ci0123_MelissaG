#include "ConsultaServidor.h"
#include "ConexionBodega.h"

#include <iostream>

bool consultarServidor(TablaRutas& rutas, const ServidorRuta& servidor, int tipo,
                       const std::vector<std::string>& campos, Mensaje& respuestaMsg) {
    std::string enviado = construirMensaje(ID_INTERMEDIARIO, servidor.id, tipo, campos);
    std::cout << "[INT -> " << servidor.id << "] " << enviado << "\n";

    std::string texto;
    int reintentos = 0;
    ConexionBodega conexion(servidor.ip, servidor.puerto);
    if (!conexion.enviar(enviado, texto, reintentos)) {
        // mismos datos que el mensaje 92: componente que no responde, tipo pendiente y reintentos
        std::cout << "[INT] sin respuesta de " << servidor.id << " al tipo " << tipo << " despues de "
                  << reintentos << " reintentos, sale de la tabla de rutas hasta su proximo 83\n";
        rutas.marcarSinRespuesta(servidor.id);
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
    rutas.marcarRespondio(servidor.id);
    return true;
}
