#include "ConsultaServidor.h"
#include "ConexionBodega.h"

#include <iostream>

bool consultarServidor(TablaRutas& rutas, const ServidorRuta& servidor, int tipo,
                       const std::vector<std::string>& campos, Mensaje& respuestaMsg) {
    std::string enviado = construirMensaje(ID_INTERMEDIARIO, servidor.id, tipo, campos);

    // del 78 solo se anota cuantas lineas trae: copiar la bitacora del servidor dentro de la
    // del intermediario la duplicaria
    bool resumir = tipo == TIPO_REQUEST_LOG;
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
    if (!resumir) {
        std::cout << "[" << servidor.id << " -> INT] " << texto << "\n";
    } else {
        size_t inicio = texto.find("/78/");
        std::string count = inicio == std::string::npos ? "?" : texto.substr(inicio + 4, texto.find('/', inicio + 4) - inicio - 4);
        std::cout << "[" << servidor.id << " -> INT] 78 con " << count << " lineas de su bitacora\n";
    }

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
