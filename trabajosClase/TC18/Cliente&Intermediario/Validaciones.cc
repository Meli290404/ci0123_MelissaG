#include "Validaciones.h"
#include "Protocolo.h"
#include <cctype>

static bool esDigitos(const std::string& s, size_t desde, size_t hasta) {
    for (size_t i = desde; i < hasta; i++) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    return true;
}

bool validarOrigenDestino(const std::string& s) {
    if (s == "BROADCAST") return true;
    // SERV_NN tiene un caracter mas que CLI_NN e INT_NN
    if (s.size() == 7 && s.substr(0, 5) == "SERV_") return esDigitos(s, 5, 7);
    if (s.size() != 6) return false;
    std::string prefijo = s.substr(0, 3);
    if (prefijo != "CLI" && prefijo != "INT") return false;
    if (s[3] != '_') return false;
    return esDigitos(s, 4, 6);
}

bool validarNumeroServidor(const std::string& s) {
    if (s.empty() || s.size() > 2 || !esDigitos(s, 0, s.size())) return false;
    return std::stoi(s) >= 1;
}

bool validarIp(const std::string& s) {
    int partes = 0;
    size_t inicio = 0;
    while (true) {
        size_t punto = s.find('.', inicio);
        std::string parte = s.substr(inicio, punto == std::string::npos ? std::string::npos : punto - inicio);
        if (parte.empty() || parte.size() > 3 || !esDigitos(parte, 0, parte.size()) || std::stoi(parte) > 255) {
            return false;
        }
        partes++;
        if (punto == std::string::npos) break;
        inicio = punto + 1;
    }
    return partes == 4;
}

bool validarPuerto(const std::string& s) {
    if (s.empty() || s.size() > 5 || !esDigitos(s, 0, s.size())) return false;
    int puerto = std::stoi(s);
    return puerto >= 1024 && puerto <= 65535;
}

bool validarTipoMensaje(const std::string& s) {
    return s.size() == 2 && esDigitos(s, 0, 2);
}

bool validarCategoria(const std::string& s) {
    if (s.size() < 3 || s.size() > 20) return false;
    for (char c : s) if (!isalpha((unsigned char)c)) return false;
    return true;
}

bool validarProducto(const std::string& s) {
    if (s.empty() || s.size() > 50) return false;
    for (char c : s) if (!isalnum((unsigned char)c) && c != '-') return false;
    return true;
}

bool validarPrecio(const std::string& s) {
    if (s.empty() || s.size() > 8) return false;
    size_t punto = s.find('.');
    std::string parteEntera = (punto == std::string::npos) ? s : s.substr(0, punto);
    if (parteEntera.empty() || parteEntera.size() > 6 || !esDigitos(parteEntera, 0, parteEntera.size())) return false;
    if (punto != std::string::npos) {
        std::string parteDecimal = s.substr(punto + 1);
        if (parteDecimal.empty() || parteDecimal.size() > 2 || !esDigitos(parteDecimal, 0, parteDecimal.size())) return false;
    }
    return true;
}

bool validarStock(const std::string& s) {
    return !s.empty() && s.size() <= 5 && esDigitos(s, 0, s.size());
}

bool validarDescripcion(const std::string& s) {
    return s.size() <= 100;
}

bool validarCount(const std::string& s) {
    return !s.empty() && s.size() <= 3 && esDigitos(s, 0, s.size());
}
