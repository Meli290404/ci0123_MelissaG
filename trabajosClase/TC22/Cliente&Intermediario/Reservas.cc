#include "Reservas.h"
#include "Mancomunado.h"

std::string Reservas::llave(const std::string& ipIsla, const std::string& producto) {
    return ipIsla + "/" + aMinusculas(producto);
}

void Reservas::agregar(const std::string& ipIsla, const std::string& producto, const std::string& servidor,
                       int cantidad, uint32_t precioCentavos) {
    std::lock_guard<std::mutex> lock(mutex_);
    time_t vence = time(nullptr) + SEGUNDOS_VIGENCIA;
    auto it = reservas_.find(llave(ipIsla, producto));
    if (it != reservas_.end()) {
        it->second.cantidad += cantidad;
        it->second.vence = vence;
        return;
    }
    reservas_[llave(ipIsla, producto)] = {ipIsla, producto, servidor, cantidad, precioCentavos, vence};
}

std::string Reservas::servidorDe(const std::string& ipIsla, const std::string& producto) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reservas_.find(llave(ipIsla, producto));
    if (it == reservas_.end() || it->second.vence < time(nullptr)) return "";
    return it->second.servidor;
}

bool Reservas::comprar(const std::string& ipIsla, const std::vector<ItemCompra>& items,
                       std::vector<Reserva>& comprados, std::string& faltante) {
    std::lock_guard<std::mutex> lock(mutex_);
    time_t ahora = time(nullptr);

    // primero se suma lo que se pide de cada producto, por si viene repetido en el 40
    std::map<std::string, int> pedido;
    for (const auto& item : items) pedido[llave(ipIsla, item.producto)] += item.cantidad;

    for (const auto& item : items) {
        auto it = reservas_.find(llave(ipIsla, item.producto));
        if (it == reservas_.end() || it->second.vence < ahora || it->second.cantidad < pedido[it->first]) {
            faltante = item.producto;
            return false;
        }
    }

    // todo alcanza: se descuenta de cada reserva. lo que sobre sigue reservado hasta que venza
    comprados.clear();
    for (const auto& item : items) {
        Reserva& reserva = reservas_[llave(ipIsla, item.producto)];
        reserva.cantidad -= item.cantidad;
        Reserva comprado = reserva;
        comprado.cantidad = item.cantidad;
        comprados.push_back(comprado);
    }
    for (auto it = reservas_.begin(); it != reservas_.end();) {
        if (it->second.cantidad == 0) it = reservas_.erase(it);
        else ++it;
    }
    return true;
}

std::vector<Reserva> Reservas::sacarVencidas() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Reserva> vencidas;
    time_t ahora = time(nullptr);
    for (auto it = reservas_.begin(); it != reservas_.end();) {
        if (it->second.vence < ahora) {
            vencidas.push_back(it->second);
            it = reservas_.erase(it);
        } else {
            ++it;
        }
    }
    return vencidas;
}

std::vector<Reserva> Reservas::todas() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Reserva> lista;
    for (const auto& par : reservas_) lista.push_back(par.second);
    return lista;
}
