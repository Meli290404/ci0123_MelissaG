#include "TablaIslas.h"

bool TablaIslas::yaVisto(const std::string& idIsla, const std::string& idMensaje) {
    std::lock_guard<std::mutex> lock(mutex_);
    time_t ahora = time(nullptr);

    // se olvidan los anuncios viejos: el ID_MSG da la vuelta en 9999 y una isla que se
    // reinicia vuelve a usar numeros, asi que no se pueden guardar para siempre
    for (auto it = vistos_.begin(); it != vistos_.end();) {
        if (ahora - it->second > SEGUNDOS_DUPLICADO) it = vistos_.erase(it);
        else ++it;
    }

    std::string llave = idIsla + "/" + idMensaje;
    if (vistos_.count(llave)) return true;
    vistos_[llave] = ahora;
    return false;
}

TablaIslas::Resultado TablaIslas::actualizar(const std::string& id, const std::string& ip, int puerto) {
    std::lock_guard<std::mutex> lock(mutex_);
    Resultado resultado = NUEVA;
    auto it = islas_.find(id);
    if (it != islas_.end() && it->second.activa) {
        resultado = (it->second.ip == ip && it->second.puerto == puerto) ? IGUAL : CAMBIO;
    }
    islas_[id] = {id, ip, puerto, time(nullptr), true};
    return resultado;
}

bool TablaIslas::darDeBaja(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = islas_.find(id);
    if (it == islas_.end() || !it->second.activa) return false;
    it->second.activa = false;
    return true;
}

std::vector<IslaRuta> TablaIslas::todas() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IslaRuta> lista;
    for (const auto& par : islas_) lista.push_back(par.second);
    return lista;
}

std::vector<IslaRuta> TablaIslas::activas() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IslaRuta> lista;
    for (const auto& par : islas_) {
        if (par.second.activa) lista.push_back(par.second);
    }
    return lista;
}
