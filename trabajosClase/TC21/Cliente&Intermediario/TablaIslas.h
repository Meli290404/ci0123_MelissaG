#ifndef TABLAISLAS_H
#define TABLAISLAS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <ctime>

// otra isla que se anuncio con un 60 del protocolo mancomunado
struct IslaRuta {
    std::string id;          // INT_NN
    std::string ip;          // la que trae el 60, donde atiende por tcp
    int puerto;              // el que trae el 60
    time_t ultimoAnuncio;    // ultimo 60 que se recibio de ella
    bool activa;             // false despues de su 61
};

// tabla de las otras islas. la llena el hilo que escucha el puerto udp 9090 y la leen
// los hilos que atienden clientes, por eso todo pasa por mutex_
class TablaIslas {
public:
    // un mismo 60 (id_isla y ID_MSG) que llega otra vez dentro de este tiempo es un duplicado
    static const int SEGUNDOS_DUPLICADO = 60;

    // devuelve true si ese anuncio ya se recibio hace poco; si no, lo anota
    bool yaVisto(const std::string& idIsla, const std::string& idMensaje);

    // resultado de registrar un 60: la isla es nueva (o vuelve despues de su 61), cambio su ip o
    // su puerto, o es la misma de siempre
    enum Resultado { NUEVA, CAMBIO, IGUAL };
    Resultado actualizar(const std::string& id, const std::string& ip, int puerto);

    // la isla mando su 61. devuelve false si no estaba o ya estaba de baja
    bool darDeBaja(const std::string& id);

    std::vector<IslaRuta> todas();
    std::vector<IslaRuta> activas();

private:
    std::mutex mutex_;
    std::map<std::string, IslaRuta> islas_;
    std::map<std::string, time_t> vistos_;   // "id_isla/ID_MSG" -> cuando llego
};

#endif
