#include "Mancomunado.h"

#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>

static bool soloDigitos(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit((unsigned char)c)) return false;
    }
    return true;
}

std::string armarMancomunado(int tipo, const std::vector<std::string>& campos) {
    char tipoTexto[4];
    snprintf(tipoTexto, sizeof(tipoTexto), "%02d", tipo);
    std::string linea = tipoTexto;
    for (const auto& campo : campos) linea += "/" + campo;
    return linea;
}

MensajeMancomunado parsearMancomunado(const std::string& linea) {
    // en tcp el mensaje puede traer el '\n' final; no es parte del ultimo campo
    std::string limpia = linea;
    while (!limpia.empty() && (limpia.back() == '\n' || limpia.back() == '\r')) limpia.pop_back();

    std::vector<std::string> partes;
    std::stringstream ss(limpia);
    std::string parte;
    while (std::getline(ss, parte, '/')) partes.push_back(parte);

    // el tipo va primero y son dos digitos, para poder rechazar rapido un mensaje invalido
    if (partes.empty() || partes[0].size() != 2 || !soloDigitos(partes[0])) {
        throw std::invalid_argument("tipo de mensaje invalido");
    }

    MensajeMancomunado msg;
    msg.tipo = std::stoi(partes[0]);
    msg.campos.assign(partes.begin() + 1, partes.end());
    return msg;
}

bool validarIdMensaje(const std::string& s) {
    return s.size() == 4 && soloDigitos(s);
}

bool validarIdIsla(const std::string& s) {
    return s.size() == 6 && s.compare(0, 4, "INT_") == 0 && soloDigitos(s.substr(4));
}

bool validarPuertoMancomunado(const std::string& s) {
    if (s.size() > 5 || !soloDigitos(s)) return false;
    int puerto = std::stoi(s);
    return puerto >= 1 && puerto <= 65535;
}
