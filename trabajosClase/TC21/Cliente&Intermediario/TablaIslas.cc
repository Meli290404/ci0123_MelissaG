#include "TablaIslas.h"
#include "Mancomunado.h"

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
        bool igual = it->second.ip == ip && it->second.puerto == puerto && !it->second.sinRespuesta;
        resultado = igual ? IGUAL : CAMBIO;
    }
    // si sigue en la misma ip y puerto se conservan las categorias; si no, se vuelven a pedir
    std::vector<std::string> categorias;
    if (resultado == IGUAL) categorias = it->second.categorias;
    islas_[id] = {id, ip, puerto, time(nullptr), true, false, categorias};
    return resultado;
}

bool TablaIslas::darDeBaja(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = islas_.find(id);
    if (it == islas_.end() || !it->second.activa) return false;
    it->second.activa = false;
    return true;
}

void TablaIslas::marcarSinRespuesta(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = islas_.find(id);
    if (it != islas_.end()) it->second.sinRespuesta = true;
}

void TablaIslas::guardarCategorias(const std::string& id, const std::vector<std::string>& categorias) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = islas_.find(id);
    if (it != islas_.end()) it->second.categorias = categorias;
}

void TablaIslas::guardarProductos(const std::string& id, const std::vector<ProductoDeIsla>& productos) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& p : productos) productos_[id + "/" + aMinusculas(p.nombre)] = p;
}

std::vector<ProductoDeIsla> TablaIslas::buscarProducto(const std::string& nombre) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProductoDeIsla> encontrados;
    for (const auto& par : productos_) {
        auto isla = islas_.find(par.second.isla);
        bool activa = isla != islas_.end() && isla->second.activa && !isla->second.sinRespuesta;
        if (activa && aMinusculas(par.second.nombre) == aMinusculas(nombre)) encontrados.push_back(par.second);
    }
    return encontrados;
}

std::string TablaIslas::estado(const IslaRuta& isla) {
    if (!isla.activa) return "de-baja";
    if (isla.sinRespuesta) return "sin-respuesta";
    return "activa";
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
        if (par.second.activa && !par.second.sinRespuesta) lista.push_back(par.second);
    }
    return lista;
}
