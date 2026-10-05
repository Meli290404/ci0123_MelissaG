#include "Protocolo.h"
#include <sstream>
#include <stdexcept>

std::string construirMensaje(const std::string& origen, const std::string& destino,
                              int tipo, const std::vector<std::string>& campos) {
    std::ostringstream oss;
    oss << origen << "|" << destino << "/" << tipo;
    for (const auto& campo : campos) oss << "/" << campo;
    return oss.str();
}

Mensaje parsearMensaje(const std::string& texto) {
    Mensaje msg;

    size_t pipe = texto.find('|');
    if (pipe == std::string::npos) {
        throw std::runtime_error("parsearMensaje: falta '|' entre origen y destino");
    }
    msg.origen = texto.substr(0, pipe);

    std::string resto = texto.substr(pipe + 1);
    size_t slash1 = resto.find('/');
    if (slash1 == std::string::npos) {
        throw std::runtime_error("parsearMensaje: falta '/' despues del destino");
    }
    msg.destino = resto.substr(0, slash1);

    resto = resto.substr(slash1 + 1);
    size_t slash2 = resto.find('/');
    std::string tipoStr = (slash2 == std::string::npos) ? resto : resto.substr(0, slash2);
    try {
        msg.tipo = std::stoi(tipoStr);
    } catch (...) {
        throw std::runtime_error("parsearMensaje: tipo_mensaje invalido");
    }

    if (slash2 != std::string::npos) {
        std::string campos = resto.substr(slash2 + 1);
        std::stringstream ss(campos);
        std::string campo;
        while (std::getline(ss, campo, '/')) msg.campos.push_back(campo);
    }

    return msg;
}