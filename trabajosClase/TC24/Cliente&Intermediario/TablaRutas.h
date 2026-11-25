#ifndef TABLARUTAS_H
#define TABLARUTAS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <ctime>

// un servidor de productos que el intermediario descubrio por broadcast (80 o 83)
struct ServidorRuta {
    std::string id;                       // SERV_NN
    std::string ip;                       // de donde llego el datagrama
    int puerto;                           // puerto tcp donde atiende 10, 20 y 50
    std::vector<std::string> categorias;
    time_t ultimoContacto;                // ultimo 80 u 83, o ultima respuesta correcta
    bool sinRespuesta;                    // no contesto la ultima vez que se le escribio
    unsigned long orden;                  // en que orden entro a la tabla; decide quien se queda una categoria repetida
};

// tabla de rutas del intermediario: que servidores de productos hay y que categorias tiene
// cada uno. la llenan los hilos de descubrimiento y la leen los hilos que atienden clientes,
// por eso todo pasa por mutex_
class TablaRutas {
public:
    // un servidor que no contesta el 82 en este tiempo se considera vencido
    static const int SEGUNDOS_VIGENCIA = 30;

    // resultado de actualizar con un 80 u 83: el servidor es nuevo, cambio algo
    // (ip, puerto, categorias o volvio despues de estar caido), o es el mismo de siempre
    enum Resultado { NUEVO, CAMBIO, IGUAL };
    Resultado actualizar(const std::string& id, const std::string& ip, int puerto,
                         const std::vector<std::string>& categorias);

    // el servidor aviso con un 72 que se apaga. solo se acepta si viene de la misma ip que tiene
    // en la tabla, para que otro equipo no pueda sacarlo. devuelve false si no estaba o no coincide
    bool eliminar(const std::string& id, const std::string& ip);

    // categorias que ese servidor anuncia y que ya tiene otro servidor activo que se registro antes.
    // cada texto queda como "categoria (SERV_NN)"
    std::vector<std::string> categoriasRepetidas(const std::string& id);

    // el intermediario le escribio y no contesto: no se usa hasta su proximo 83
    void marcarSinRespuesta(const std::string& id);
    void marcarRespondio(const std::string& id);

    // todos los servidores descubiertos, aunque esten vencidos
    std::vector<ServidorRuta> todos();

    // solo los activos: vigentes y que contestaron la ultima vez
    std::vector<ServidorRuta> activos();

    // servidor activo dueno de la categoria. si dos la anuncian, la ruta es del que se registro primero.
    // devuelve una lista vacia o de un solo servidor
    std::vector<ServidorRuta> conCategoria(const std::string& categoria);

    // "activo", "sin-respuesta" o "vencido"
    static std::string estado(const ServidorRuta& s);

private:
    std::mutex mutex_;
    std::map<std::string, ServidorRuta> servidores_;
    unsigned long siguienteOrden_ = 1;

    static bool estaActivo(const ServidorRuta& s);
};

#endif
