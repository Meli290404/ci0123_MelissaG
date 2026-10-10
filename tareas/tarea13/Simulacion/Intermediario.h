#ifndef INTERMEDIARIO_H
#define INTERMEDIARIO_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <condition_variable>

#include "Buzon.h"
#include "Protocolo.h"
#include "Mancomunado.h"

// isla conocida por su anuncio MAN_ANNOUNCE
struct IslaRemota {
    std::string idIsla;
    std::string ip;
    std::string puerto;
};

// producto de otra isla visto en una respuesta a MAN_GET_PRODUCTS
struct ProductoRemoto {
    int idIsla;
    float precio;
};

// reserva que otra isla hizo en esta con MAN_RESERVE y que MAN_BUY consume
struct Reserva {
    int cantidad;
    float precio;
    std::chrono::steady_clock::time_point hora;
};

// texto de una respuesta mancomunada y la explicacion para el rotulo;
// texto vacio significa que no se responde
struct RespuestaIsla {
    std::string texto;
    std::string explicacion;
};

class Intermediario {
public:
    // idsIslas son los intermediarios que reciben el broadcast, incluido este
    Intermediario( Buzon & buzon, int miId, const std::string & idTexto, const std::string & ip,
                    int cantidadBodegas, std::vector<int> idsIslas );

    void iniciar();                 // registra sus bodegas y abre la escucha del puerto 9090
    void anunciarse();              // MAN_ANNOUNCE por broadcast
    void conocerIslas();            // espera los anuncios de las otras islas y les pide MAN_GET_CATEGORIES
    void atenderCliente();          // protocolo interno con el cliente hasta recibir MSG_DEATH
    void probarIsla( int idIsla );  // guion de solicitudes mancomunadas, incluidos los casos de error
    void despedirse();              // MAN_ANNOUNCE_BYE por broadcast y fin de la escucha
    void terminar();                // MSG_DEATH a sus bodegas

private:
    Buzon & buzon_;
    int miId_;
    std::string idTexto_;
    std::string ip_;
    int cantidadBodegas_;
    std::vector<int> idsIslas_;
    int siguienteIdMensaje_ = 1;
    std::thread escucha_;

    // sus propias bodegas, protocolo interno
    std::vector<int> idsBodega_;                 // ordenadas por id
    std::map<std::string, int> categoriaBodega_; // categoria -> id numerico de la bodega duena
    std::map<int, std::string> nombreBodega_;    // id numerico -> nombre (para los rotulos)
    std::mutex mtxBodegas_;                      // la escucha y la atencion del cliente usan las mismas bodegas

    // otras islas, protocolo mancomunado
    std::map<int, IslaRemota> islas_;            // id numerico del intermediario -> datos de su anuncio
    std::set<std::string> anunciosVistos_;       // id_isla/ID_MSG ya procesados, para descartar duplicados
    std::mutex mtxIslas_;                        // islas_ lo cambia la escucha y lo lee la atencion del cliente
    std::condition_variable cvIslas_;
    std::map<std::string, int> islaDeCategoria_;           // categoria remota -> isla que la tiene
    std::map<std::string, ProductoRemoto> productoRemoto_; // producto remoto -> isla y precio

    // reservas que otras islas hicieron en esta, por id de la isla que reservo; solo las usa la escucha
    std::map<int, std::map<std::string, Reserva>> reservas_;

    std::vector<ItemCarrito> carrito_;

    // protocolo interno con las bodegas propias
    void registrarBodegas();
    bool consultarBodega( int idBodega, int tipo, const std::vector<std::string> & campos,
                          const std::string & explicacion, MensajeProtocolo & respuesta );
    bool buscarProductoLocal( const std::string & nombreMinusculas, int & idBodega, std::string & nombre,
                              float & precio, int & stock );

    // protocolo interno con el cliente
    void manejarCategorias();
    void manejarProductos( const std::string & categoria );
    void manejarAgregar( const std::string & producto, const std::string & cantidadTexto );
    void agregarRemoto( const std::string & producto, const std::string & cantidadTexto );
    void manejarFactura( const std::vector<std::string> & campos );
    bool campoValido( const std::string & tipoOriginal, const std::string & campo, const std::string & valor,
                      size_t tamMax, bool ( *validador )( const std::string & ) );
    void responderCliente( int tipo, const std::vector<std::string> & campos, const std::string & explicacion );

    // protocolo mancomunado, lado de quien pregunta
    void enviarBroadcast( const std::string & texto );
    bool solicitarIsla( int idIsla, const std::string & texto, const std::string & explicacion,
                        MensajeMancomunado & respuesta );
    bool buscarProductoRemoto( const std::string & producto );
    void recordarProductos( int idIsla, const std::string & lista );
    std::string textoIsla( int idIsla );

    // protocolo mancomunado, lado de la isla que responde (hilo escucha_)
    void escuchar();
    void recibirAnuncio( const MensajeMancomunado & mm, int idOrigen, const std::string & linea );
    bool recibirDespedida( const MensajeMancomunado & mm, int idOrigen, const std::string & linea );
    RespuestaIsla atenderSolicitud( const MensajeMancomunado & mm, int idOrigen );
    RespuestaIsla atenderCategoriasRemotas();
    RespuestaIsla atenderProductosRemotos( const std::vector<std::string> & campos );
    RespuestaIsla atenderReservaRemota( const std::vector<std::string> & campos, int idOrigen );
    RespuestaIsla atenderCompraRemota( const std::vector<std::string> & campos, int idOrigen );
    void responderSolicitud( const DatosRed & solicitud, const RespuestaIsla & respuesta );
};

#endif
