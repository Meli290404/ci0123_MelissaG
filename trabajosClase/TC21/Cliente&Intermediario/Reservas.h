#ifndef RESERVAS_H
#define RESERVAS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <ctime>
#include <cstdint>

// stock que otra isla aparto con un 50 y todavia no compro con un 40. el stock ya se desconto
// en el servidor de productos; si la reserva vence, se le devuelve con un 52
struct Reserva {
    std::string ipIsla;          // ip de la conexion tcp que hizo el 50
    std::string producto;        // nombre como esta en el archivo de la bodega
    std::string servidor;        // SERV_NN donde quedo apartado
    int cantidad;
    uint32_t precioCentavos;
    time_t vence;
};

// un producto del 40: nombre como lo mando la otra isla y cantidad
struct ItemCompra {
    std::string producto;
    int cantidad;
};

// reservas de las otras islas. las usan los hilos que atienden conexiones del puerto 9090
// y el hilo que devuelve las vencidas, por eso todo pasa por mutex_
class Reservas {
public:
    // tiempo que dura una reserva segun el protocolo mancomunado
    static const int SEGUNDOS_VIGENCIA = 60;

    // si esa isla ya tenia reservado el producto, se suma la cantidad y se renueva el plazo
    void agregar(const std::string& ipIsla, const std::string& producto, const std::string& servidor,
                 int cantidad, uint32_t precioCentavos);

    // servidor donde esa isla ya tiene reservado el producto, o "" si no tiene reserva vigente
    std::string servidorDe(const std::string& ipIsla, const std::string& producto);

    // revisa que cada producto tenga una reserva vigente de esa isla que alcance. si todos alcanzan,
    // descuenta las cantidades y deja en comprados el nombre real y el precio de cada uno; si uno
    // no alcanza no toca nada y deja su nombre en faltante
    bool comprar(const std::string& ipIsla, const std::vector<ItemCompra>& items,
                 std::vector<Reserva>& comprados, std::string& faltante);

    // saca y devuelve las reservas cuyo plazo ya paso, para devolver el stock al servidor
    std::vector<Reserva> sacarVencidas();

    std::vector<Reserva> todas();

private:
    std::mutex mutex_;
    std::map<std::string, Reserva> reservas_;   // "ip/producto en minusculas" -> reserva

    static std::string llave(const std::string& ipIsla, const std::string& producto);
};

#endif
