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
    if (s == "SERV") return true;
    if (s.size() != 6) return false;
    std::string prefijo = s.substr(0, 3);
    if (prefijo != "CLI" && prefijo != "INT") return false;
    if (s[3] != '_') return false;
    return esDigitos(s, 4, 6);
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

bool validarTamanoMensaje(const std::string& mensajeCompleto) {
    return mensajeCompleto.size() <= TAM_MAX_MENSAJE;
}