#include <iostream>
#include <thread>
#include <csignal>
#include <chrono>

#include "Socket.h"
#include "HttpUtil.h"
#include "Carritos.h"
#include "AtencionCliente.h"
#include "TablaRutas.h"
#include "Descubrimiento.h"
#include "Validaciones.h"

static const int PUERTO_CLIENTES = 8080;
static const int SEGUNDOS_ENTRE_82 = 10;   // menos que TablaRutas::SEGUNDOS_VIGENCIA
static const int MAXLINE = 4096;
static const size_t MAX_PETICION = 8192;

std::string leerPeticion(VSocket* cliente) {
    char buf[MAXLINE];
    std::string acumulado;
    // se lee hasta el fin de los encabezados o hasta pasar el limite
    while (acumulado.find("\r\n\r\n") == std::string::npos && acumulado.size() <= MAX_PETICION) {
        size_t leidos = cliente->Read(buf, MAXLINE - 1);
        if (leidos == 0) break;
        acumulado.append(buf, leidos);
    }
    return acumulado;
}

void atenderCliente(VSocket* cliente, TablaRutas* rutas, Carritos* carritos) {
    try {
        std::string peticion = leerPeticion(cliente);

        // algunos navegadores abren conexiones de reserva y las cierran sin enviar nada
        if (!peticion.empty()) {
            std::cout << "[CLI -> INT] " << peticion.substr(0, peticion.find("\r\n")) << "\n";

            AtencionCliente atencion(*rutas, *carritos);
            RespuestaHttp respuesta = atencion.responder(peticion);

            std::cout << "[INT -> CLI] http " << respuesta.codigo << "\n";
            std::string salida = construirRespuestaHttp(respuesta.codigo, respuesta.cuerpo);
            cliente->Write(salida.data(), salida.size());
        }
        cliente->Close();
    } catch (const std::exception& e) {
        std::cerr << "error atendiendo cliente: " << e.what() << "\n";
    }
    delete cliente;
}

static void mostrarAlta(TablaRutas::Resultado resultado, const std::string& id, const std::string& ip, int puerto,
                        size_t categorias) {
    if (resultado == TablaRutas::IGUAL) return;
    std::cout << "[INT] tabla de rutas: " << id << " en " << ip << ":" << puerto << " con " << categorias
              << " categorias (" << (resultado == TablaRutas::NUEVO ? "nuevo" : "actualizado") << ")\n";
}

// hilo que espera datagramas con recvFrom en el puerto udp del intermediario:
//   80: un servidor de productos que acaba de arrancar busca al intermediario; se registra y se le contesta 81
//   83: un servidor contesta nuestro 82; se registra o se refresca
void escucharServidores(Socket* udp, TablaRutas* rutas) {
    for (;;) {
        std::string ip;
        int puerto;
        std::string texto;
        try {
            texto = recibirDatagrama(*udp, ip, puerto);
        } catch (const std::exception& e) {
            std::cerr << "error recibiendo datagrama: " << e.what() << "\n";
            continue;
        }

        Mensaje msg;
        try {
            msg = parsearMensaje(texto);
        } catch (const std::exception&) {
            std::cout << "[UDP " << ip << "] datagrama descartado, no cumple el protocolo: " << texto << "\n";
            continue;
        }
        // nuestro propio 82 tambien nos llega por el broadcast
        if (msg.tipo == TIPO_BUSCO_SERVIDORES) continue;
        if (msg.tipo != TIPO_BUSCO_INTERMEDIARIO && msg.tipo != TIPO_SOY_SERVIDOR) {
            std::cout << "[UDP " << ip << "] tipo " << msg.tipo << " no se espera por udp, se ignora\n";
            continue;
        }
        if (msg.destino != ID_INTERMEDIARIO && msg.destino != ID_BROADCAST) continue;

        int puertoTcp;
        std::vector<std::string> categorias;
        if (msg.origen.rfind("SERV_", 0) != 0 || !leerAnuncio(msg, puertoTcp, categorias)) {
            std::cout << "[UDP " << ip << "] anuncio mal formado, se ignora: " << texto << "\n";
            continue;
        }

        // en el 83 periodico solo se muestra el datagrama si algo cambio, para no llenar la pantalla
        TablaRutas::Resultado resultado = rutas->actualizar(msg.origen, ip, puertoTcp, categorias);
        if (msg.tipo == TIPO_BUSCO_INTERMEDIARIO || resultado != TablaRutas::IGUAL) {
            std::cout << "[UDP " << ip << " -> INT] " << texto << "\n";
        }
        mostrarAlta(resultado, msg.origen, ip, puertoTcp, categorias.size());

        if (msg.tipo == TIPO_BUSCO_INTERMEDIARIO) {
            std::string respuesta = construirMensaje(ID_INTERMEDIARIO, msg.origen, TIPO_SOY_INTERMEDIARIO,
                                                     {resultado == TablaRutas::NUEVO ? "nuevo" : "actualizado"});
            try {
                enviarDatagrama(*udp, respuesta, ip, PUERTO_UDP_SERVIDORES);
                std::cout << "[UDP INT -> " << ip << "] " << respuesta << "\n";
            } catch (const std::exception& e) {
                std::cerr << "no se pudo contestar el 80: " << e.what() << "\n";
            }
        }
    }
}

// hilo que manda el 82 por broadcast al arrancar y luego cada SEGUNDOS_ENTRE_82 segundos;
// asi se descubren los servidores que arrancaron antes que el intermediario y se sabe cuales siguen vivos
void llamarServidores(Socket* udp, std::string broadcast) {
    std::string mensaje = construirMensaje(ID_INTERMEDIARIO, ID_BROADCAST, TIPO_BUSCO_SERVIDORES, {});
    bool primera = true;
    for (;;) {
        try {
            enviarDatagrama(*udp, mensaje, broadcast, PUERTO_UDP_SERVIDORES);
            if (primera) std::cout << "[UDP INT -> " << broadcast << "] " << mensaje << " (se repite cada "
                                   << SEGUNDOS_ENTRE_82 << " s)\n";
            primera = false;
        } catch (const std::exception& e) {
            std::cerr << "no se pudo mandar el 82: " << e.what() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(SEGUNDOS_ENTRE_82));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "uso: " << argv[0] << " <ip donde escucha a los clientes> <ip de la red de los servidores> [broadcast]\n";
        return 1;
    }
    const char* ipClientes = argv[1];
    const char* ipPrivada = argv[2];
    if (!validarIp(ipClientes) || !validarIp(ipPrivada)) {
        std::cerr << "las ips deben ser ipv4\n";
        return 1;
    }

    // el broadcast sale de la ip de la red de los servidores (en la isla 4, 172.16.123.79)
    std::string broadcast = argc >= 4 ? argv[3] : broadcastDe(ipPrivada);
    if (broadcast.empty() || !validarIp(broadcast)) {
        std::cerr << "no se encontro la direccion de broadcast de " << ipPrivada << ", pasela como tercer argumento\n";
        return 1;
    }

    // si un cliente cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    // las dos se comparten entre todos los hilos y cada una protege sus datos con su propio mutex
    TablaRutas rutas;
    Carritos carritos;

    VSocket* servidor = new Socket('s');
    Socket* udp = new Socket('d');
    try {
        servidor->Bind(ipClientes, PUERTO_CLIENTES);
        servidor->MarkPassive(10);

        // el socket udp se liga a todas las interfaces: si se liga a la ip no le llegan los broadcast
        udp->Bind(PUERTO_UDP_INTERMEDIARIO);
        udp->HabilitarBroadcast();
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el intermediario: " << e.what() << "\n";
        delete servidor;
        delete udp;
        return 1;
    }

    std::cout << "intermediario escuchando en http://" << ipClientes << ":" << PUERTO_CLIENTES << "/\n";
    std::cout << "descubrimiento por udp en el puerto " << PUERTO_UDP_INTERMEDIARIO << ", broadcast " << broadcast << "\n";
    std::cout << "tabla de rutas en http://" << ipClientes << ":" << PUERTO_CLIENTES << "/rutas\n";

    std::thread escucha(escucharServidores, udp, &rutas);
    escucha.detach();
    std::thread llamada(llamarServidores, udp, broadcast);
    llamada.detach();

    for (;;) {
        VSocket* cliente = servidor->AcceptConnection();
        std::thread trabajador(atenderCliente, cliente, &rutas, &carritos);
        trabajador.detach();
    }

    return 0;
}
