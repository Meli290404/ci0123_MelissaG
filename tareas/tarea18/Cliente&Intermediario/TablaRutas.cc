#include "TablaRutas.h"

TablaRutas::Resultado TablaRutas::actualizar(const std::string& id, const std::string& ip, int puerto,
                                             const std::vector<std::string>& categorias) {
    std::lock_guard<std::mutex> lock(mutex_);
    Resultado resultado = NUEVO;
    auto it = servidores_.find(id);
    if (it != servidores_.end()) {
        const ServidorRuta& antes = it->second;
        bool igual = antes.ip == ip && antes.puerto == puerto && antes.categorias == categorias && estaActivo(antes);
        resultado = igual ? IGUAL : CAMBIO;
    }
    servidores_[id] = {id, ip, puerto, categorias, time(nullptr), false};
    return resultado;
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
            if (c == categoria) {
                lista.push_back(s);
                break;
            }
        }
    }
    return lista;
}
