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
    bool sinRespuesta;       // no contesto la ultima consulta tcp; no se usa hasta su proximo 60
    std::vector<std::string> categorias;   // las de su ultimo 01 a un 10, en minusculas
};

// un producto que otra isla mostro en un 01 a un 20
struct ProductoDeIsla {
    std::string isla;        // INT_NN
    std::string nombre;      // como lo escribio la otra isla
    std::string precio;      // como texto, tal cual llego
    int stock;
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
    // su puerto (o vuelve despues de no contestar), o es la misma de siempre
    enum Resultado { NUEVA, CAMBIO, IGUAL };
    Resultado actualizar(const std::string& id, const std::string& ip, int puerto);

    // la isla mando su 61. devuelve false si no estaba o ya estaba de baja
    bool darDeBaja(const std::string& id);

    // la isla no contesto despues de todos los intentos: se deshabilita hasta su proximo 60
    void marcarSinRespuesta(const std::string& id);

    void guardarCategorias(const std::string& id, const std::vector<std::string>& categorias);

    // guarda o actualiza los productos que esa isla mostro
    void guardarProductos(const std::string& id, const std::vector<ProductoDeIsla>& productos);

    // productos que alguna isla activa mostro con ese nombre, sin importar mayusculas
    std::vector<ProductoDeIsla> buscarProducto(const std::string& nombre);

    std::vector<IslaRuta> todas();

    // las que no se han despedido y contestaron su ultima consulta
    std::vector<IslaRuta> activas();

    // "activa", "sin-respuesta" o "de-baja"
    static std::string estado(const IslaRuta& isla);

private:
    std::mutex mutex_;
    std::map<std::string, IslaRuta> islas_;
    std::map<std::string, time_t> vistos_;   // "id_isla/ID_MSG" -> cuando llego
    std::map<std::string, ProductoDeIsla> productos_;   // "id_isla/producto en minusculas" -> producto
};

#endif
