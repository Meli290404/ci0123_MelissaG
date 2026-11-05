#include "Descubrimiento.h"
#include "Validaciones.h"

#include <sstream>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static std::string ipDe(const struct sockaddr_in& direccion) {
    char texto[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &direccion.sin_addr, texto, sizeof(texto));
    return texto;
}

std::string broadcastDe(const std::string& ip) {
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == -1) return "";

    // se busca la interfaz que tiene esa ip y se toma su direccion de broadcast
    std::string broadcast;
    for (struct ifaddrs* i = interfaces; i != nullptr && broadcast.empty(); i = i->ifa_next) {
        if (i->ifa_addr == nullptr || i->ifa_addr->sa_family != AF_INET) continue;
        if (!(i->ifa_flags & IFF_BROADCAST) || i->ifa_broadaddr == nullptr) continue;
        if (ipDe(*(struct sockaddr_in*)i->ifa_addr) == ip) broadcast = ipDe(*(struct sockaddr_in*)i->ifa_broadaddr);
    }
    freeifaddrs(interfaces);
    return broadcast;
}

void enviarDatagrama(Socket& udp, const std::string& mensaje, const std::string& ip, int puerto) {
    struct sockaddr_in destino;
    VSocket::ArmarDireccion(ip.c_str(), puerto, &destino);
    udp.sendTo(mensaje.data(), mensaje.size(), &destino);
}

std::string recibirDatagrama(Socket& udp, std::string& ip, int& puerto) {
    char buffer[2048];
    struct sockaddr_in origen;
    size_t leidos = udp.recvFrom(buffer, sizeof(buffer), &origen);
    ip = ipDe(origen);
    puerto = ntohs(origen.sin_port);
    return std::string(buffer, leidos);
}

std::vector<std::string> camposAnuncio(int puerto, const std::vector<std::string>& categorias) {
    std::vector<std::string> campos = {std::to_string(puerto), std::to_string(categorias.size())};
    if (!categorias.empty()) {
        std::string lista;
        for (const auto& c : categorias) lista += (lista.empty() ? "" : ",") + c;
        campos.push_back(lista);
    }
    return campos;
}

bool leerAnuncio(const Mensaje& msg, int& puerto, std::vector<std::string>& categorias) {
    if (msg.campos.size() < 2 || !validarPuerto(msg.campos[0]) || !validarCount(msg.campos[1])) return false;

    categorias.clear();
    if (msg.campos.size() >= 3) {
        std::stringstream ss(msg.campos[2]);
        std::string categoria;
        while (std::getline(ss, categoria, ',')) {
            if (!validarCategoria(categoria)) return false;
            categorias.push_back(categoria);
        }
    }
    // la cantidad declarada debe coincidir con la lista
    if ((int)categorias.size() != std::stoi(msg.campos[1])) return false;

    puerto = std::stoi(msg.campos[0]);
    return true;
}
