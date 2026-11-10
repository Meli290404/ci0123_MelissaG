#include "Carritos.h"

int Carritos::nuevaSesion() {
    std::lock_guard<std::mutex> lock(mutex_);
    int sesion = siguienteSesion_++;
    carritos_[sesion] = {};
    return sesion;
}

bool Carritos::existe(int sesion) {
    std::lock_guard<std::mutex> lock(mutex_);
    return carritos_.count(sesion) > 0;
}

bool Carritos::agregar(int sesion, const ItemReservado& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = carritos_.find(sesion);
    if (it == carritos_.end()) return false;

    for (auto& existente : it->second) {
        if (existente.producto == item.producto && existente.precioCentavos == item.precioCentavos) {
            existente.cantidad += item.cantidad;
            return true;
        }
    }
    it->second.push_back(item);
    return true;
}

bool Carritos::vaciar(int sesion, std::vector<ItemReservado>& items) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = carritos_.find(sesion);
    if (it == carritos_.end()) return false;

    items = it->second;
    it->second.clear();
    return true;
}