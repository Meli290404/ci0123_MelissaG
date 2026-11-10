#include "TablaRutas.h"

TablaRutas::Resultado TablaRutas::actualizar(const std::string& id, const std::string& ip, int puerto,
                                             const std::vector<std::string>& categorias) {
    std::lock_guard<std::mutex> lock(mutex_);
    Resultado resultado = NUEVO;
    unsigned long orden = 0;
    auto it = servidores_.find(id);
    if (it != servidores_.end()) {
        const ServidorRuta& antes = it->second;
        bool igual = antes.ip == ip && antes.puerto == puerto && antes.categorias == categorias && estaActivo(antes);
        resultado = igual ? IGUAL : CAMBIO;
        // mientras siga activo conserva su turno; si vuelve despues de caerse, entra como el ultimo
        if (estaActivo(antes)) orden = antes.orden;
    }
    if (orden == 0) orden = siguienteOrden_++;
    servidores_[id] = {id, ip, puerto, categorias, time(nullptr), false, orden};
    return resultado;
}

bool TablaRutas::eliminar(const std::string& id, const std::string& ip) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = servidores_.find(id);
    if (it == servidores_.end() || it->second.ip != ip) return false;
    servidores_.erase(it);
    return true;
}

std::vector<std::string> TablaRutas::categoriasRepetidas(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> repetidas;
    auto it = servidores_.find(id);
    if (it == servidores_.end()) return repetidas;

    for (const auto& categoria : it->second.categorias) {
        for (const auto& par : servidores_) {
            const ServidorRuta& otro = par.second;
            if (otro.id == id || !estaActivo(otro) || otro.orden > it->second.orden) continue;
            for (const auto& c : otro.categorias) {
                if (c == categoria) repetidas.push_back(categoria + " (" + otro.id + ")");
            }
        }
    }
    return repetidas;
}

void TablaRutas::marcarSinRespuesta(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = servidores_.find(id);
    if (it != servidores_.end()) it->second.sinRespuesta = true;
}

void TablaRutas::marcarRespondio(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = servidores_.find(id);
    if (it != servidores_.end()) {
        it->second.sinRespuesta = false;
        it->second.ultimoContacto = time(nullptr);
    }
}

bool TablaRutas::estaActivo(const ServidorRuta& s) {
    return !s.sinRespuesta && time(nullptr) - s.ultimoContacto <= SEGUNDOS_VIGENCIA;
}

std::string TablaRutas::estado(const ServidorRuta& s) {
    if (s.sinRespuesta) return "sin-respuesta";
    if (time(nullptr) - s.ultimoContacto > SEGUNDOS_VIGENCIA) return "vencido";
    return "activo";
}

std::vector<ServidorRuta> TablaRutas::todos() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ServidorRuta> lista;
    for (const auto& par : servidores_) lista.push_back(par.second);
    return lista;
}

std::vector<ServidorRuta> TablaRutas::activos() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ServidorRuta> lista;
    for (const auto& par : servidores_) {
        if (estaActivo(par.second)) lista.push_back(par.second);
    }
    return lista;
}

std::vector<ServidorRuta> TablaRutas::conCategoria(const std::string& categoria) {
    std::vector<ServidorRuta> lista;
    for (const auto& s : activos()) {
        for (const auto& c : s.categorias) {
            // se queda con el que se registro primero
            if (c == categoria && (lista.empty() || s.orden < lista[0].orden)) lista = {s};
        }
    }
    return lista;
}
