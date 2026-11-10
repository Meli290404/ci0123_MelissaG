#include <iostream>
#include <thread>
#include <csignal>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <pthread.h>
#include <unistd.h>

#include "Socket.h"
#include "HttpUtil.h"
#include "Carritos.h"
#include "AtencionCliente.h"
#include "TablaRutas.h"
#include "Descubrimiento.h"
#include "Validaciones.h"
#include "Mancomunado.h"
#include "TablaIslas.h"
#include "Reservas.h"
#include "AtencionIsla.h"
#include "ConexionIsla.h"

static const int PUERTO_CLIENTES = 8080;
static const int SEGUNDOS_ENTRE_82 = 10;   // menos que TablaRutas::SEGUNDOS_VIGENCIA
static const int VECES_ANUNCIO_INICIAL = 2;   // el mismo 60 se repite por si se pierde un datagrama
static const int SEGUNDOS_ESPERA_ISLA = 5;     // timeout recomendado por el mancomunado
static const int SEGUNDOS_ENTRE_VENCIMIENTOS = 5;
static const size_t MAX_SOLICITUD_ISLA = 65536;  // el mancomunado no tiene maximo; esto evita llenar la memoria

// datos de esta isla para el protocolo mancomunado
struct DatosIsla {
    std::string id;                        // INT_04
    std::string ip;                        // ip privada, la que va en el 60
    int puerto;                            // puerto tcp que va en el 60
    std::vector<std::string> broadcasts;   // a donde se mandan el 60 y el 61
};

// ID_MSG del 60: cada anuncio lleva uno distinto. arranca en un numero al azar para que,
// si el intermediario se reinicia, sus anuncios nuevos no parezcan duplicados de los viejos
static std::atomic<int> siguienteIdMensaje(0);

static std::string nuevoIdMensaje() {
    char texto[8];
    snprintf(texto, sizeof(texto), "%04d", siguienteIdMensaje++ % 10000);
    return texto;
}
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

void atenderCliente(VSocket* cliente, TablaRutas* rutas, TablaIslas* islas, Carritos* carritos) {
    try {
        std::string peticion = leerPeticion(cliente);

        // algunos navegadores abren conexiones de reserva y las cierran sin enviar nada
        if (!peticion.empty()) {
            std::cout << "[CLI -> INT] " << peticion.substr(0, peticion.find("\r\n")) << "\n";

            AtencionCliente atencion(*rutas, *islas, *carritos);
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
//   72: un servidor avisa que se apaga; se saca de la tabla de rutas
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
        if (msg.tipo == TIPO_BAJA_SERVIDOR) {
            std::cout << "[UDP " << ip << " -> INT] " << texto << "\n";
            if (msg.destino != ID_INTERMEDIARIO) continue;
            if (rutas->eliminar(msg.origen, ip)) {
                std::cout << "[INT] tabla de rutas: " << msg.origen << " se apago, se saca de la tabla\n";
            } else {
                std::cout << "[INT] " << msg.origen << " no esta en la tabla con la ip " << ip << ", se ignora el 72\n";
            }
            continue;
        }
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

        // cada servidor de la isla debe tener categorias distintas. si repite una, se registra igual
        // pero la ruta de esa categoria se queda con el servidor que llego primero
        if (resultado != TablaRutas::IGUAL) {
            for (const auto& repetida : rutas->categoriasRepetidas(msg.origen)) {
                std::cout << "[INT] aviso: " << msg.origen << " repite la categoria " << repetida
                          << "; la ruta se queda con el primero\n";
            }
        }

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

// manda un mismo mensaje mancomunado al broadcast de cada isla. si una direccion falla se sigue
// con las demas, porque las otras islas no tienen por que estar conectadas
static void mandarATodasLasIslas(Socket* udp, const DatosIsla& isla, const std::string& mensaje) {
    for (const auto& broadcast : isla.broadcasts) {
        try {
            enviarDatagrama(*udp, mensaje, broadcast, PUERTO_MANCOMUNADO);
        } catch (const std::exception& e) {
            std::cerr << "no se pudo mandar " << mensaje << " a " << broadcast << ": " << e.what() << "\n";
        }
    }
}

// 60/ID_MSG/INT_04/<ip>/<puerto> a todas las islas, con un ID_MSG nuevo
static void anunciarIsla(Socket* udp, const DatosIsla& isla, const std::string& motivo) {
    std::string mensaje = armarMancomunado(MAN_ANNOUNCE, {nuevoIdMensaje(), isla.id, isla.ip, std::to_string(isla.puerto)});
    mandarATodasLasIslas(udp, isla, mensaje);
    std::cout << "[MAN UDP " << isla.id << " -> " << isla.broadcasts.size() << " islas] " << mensaje
              << " (" << motivo << ")\n";
}

// hilo que anuncia la isla al arrancar. el mismo 60, con el mismo ID_MSG, se manda dos veces:
// si el primero se pierde llega el segundo, y si llegan los dos el receptor descarta el repetido
void anunciarAlArrancar(Socket* udp, DatosIsla isla) {
    std::string mensaje = armarMancomunado(MAN_ANNOUNCE, {nuevoIdMensaje(), isla.id, isla.ip, std::to_string(isla.puerto)});
    for (int vez = 1; vez <= VECES_ANUNCIO_INICIAL; vez++) {
        mandarATodasLasIslas(udp, isla, mensaje);
        std::cout << "[MAN UDP " << isla.id << " -> " << isla.broadcasts.size() << " islas] " << mensaje
                  << " (anuncio inicial, vez " << vez << ")\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// hilo corto que le pide las categorias (10) a una isla recien descubierta. va aparte para que
// la espera de la conexion tcp no detenga al hilo que escucha el udp
void conocerIsla(TablaIslas* islas, IslaRuta isla) {
    std::vector<std::string> categorias;
    if (pedirCategoriasIsla(*islas, isla, categorias)) {
        std::cout << "[INT] tabla de islas: " << isla.id << " tiene " << categorias.size() << " categorias\n";
    }
}

// hilo que espera con recvFrom los anuncios de las otras islas en el puerto udp 9090:
//   60: una isla se anuncia; se registra en la tabla de islas
//   61: una isla se despide; se da de baja
// por udp no se contesta con 02: no hay conexion por donde responder, solo se descarta
void escucharIslas(Socket* udp, TablaIslas* islas, DatosIsla isla) {
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

        MensajeMancomunado msg;
        try {
            msg = parsearMancomunado(texto);
        } catch (const std::exception&) {
            std::cout << "[MAN UDP " << ip << "] datagrama descartado, no cumple el protocolo: " << texto << "\n";
            continue;
        }

        if (msg.tipo == MAN_ANNOUNCE) {
            const auto& c = msg.campos;
            if (c.size() != 4 || !validarIdMensaje(c[0]) || !validarIdIsla(c[1]) || !validarIp(c[2]) ||
                !validarPuertoMancomunado(c[3])) {
                std::cout << "[MAN UDP " << ip << "] 60 mal formado, se descarta: " << texto << "\n";
                continue;
            }
            // nuestro propio 60 tambien llega, porque nuestra red esta en la lista de broadcasts
            if (c[1] == isla.id) continue;
            if (islas->yaVisto(c[1], c[0])) {
                std::cout << "[MAN UDP " << c[1] << " -> " << isla.id << "] " << texto << " (repetido, se descarta)\n";
                continue;
            }

            std::cout << "[MAN UDP " << c[1] << " -> " << isla.id << "] " << texto << "\n";
            TablaIslas::Resultado resultado = islas->actualizar(c[1], c[2], std::stoi(c[3]));
            if (resultado == TablaIslas::IGUAL) continue;
            std::cout << "[INT] tabla de islas: " << c[1] << " en " << c[2] << ":" << c[3] << " ("
                      << (resultado == TablaIslas::NUEVA ? "nueva" : "cambio") << ")\n";

            // la isla nueva arranco despues que nosotros y no oyo nuestro 60: se anuncia otra vez.
            // las islas que ya nos conocen lo reciben como IGUAL y no contestan, asi que no hay bucle
            if (resultado == TablaIslas::NUEVA) anunciarIsla(udp, isla, "para que " + c[1] + " nos conozca");

            IslaRuta nueva = {c[1], c[2], std::stoi(c[3]), time(nullptr), true, false, {}};
            std::thread consulta(conocerIsla, islas, nueva);
            consulta.detach();

        } else if (msg.tipo == MAN_ANNOUNCE_BYE) {
            if (msg.campos.size() != 1 || !validarIdIsla(msg.campos[0])) {
                std::cout << "[MAN UDP " << ip << "] 61 mal formado, se descarta: " << texto << "\n";
                continue;
            }
            if (msg.campos[0] == isla.id) continue;
            std::cout << "[MAN UDP " << msg.campos[0] << " -> " << isla.id << "] " << texto << "\n";
            if (islas->darDeBaja(msg.campos[0])) {
                std::cout << "[INT] tabla de islas: " << msg.campos[0] << " se despidio, queda de baja\n";
            }

        } else {
            std::cout << "[MAN UDP " << ip << "] tipo " << msg.tipo << " no va por udp, se descarta\n";
        }
    }
}

// hilo que espera ctrl+c o kill con sigwait, igual que en servidorProductos. antes de terminar
// se despide de las otras islas con un 61 para que nos den de baja
void esperarApagado(Socket* udp, DatosIsla isla, sigset_t senales) {
    int senal = 0;
    sigwait(&senales, &senal);
    std::string mensaje = armarMancomunado(MAN_ANNOUNCE_BYE, {isla.id});
    mandarATodasLasIslas(udp, isla, mensaje);
    std::cout << "[MAN UDP " << isla.id << " -> " << isla.broadcasts.size() << " islas] " << mensaje << "\n";
    std::cout << "[INT] se apaga (senal " << senal << ")\n";
    std::exit(0);
}

// lee una solicitud de otra isla. el mancomunado no dice como termina un mensaje: se lee hasta el
// '\n', hasta que la otra isla cierre su lado, o hasta que pase un segundo sin datos nuevos.
// si en SEGUNDOS_ESPERA_ISLA no llega nada, se devuelve vacio
static std::string leerSolicitudIsla(VSocket* conexion) {
    char buf[1024];
    std::string acumulado;
    conexion->SetTimeout(SEGUNDOS_ESPERA_ISLA);
    while (acumulado.find('\n') == std::string::npos && acumulado.size() <= MAX_SOLICITUD_ISLA) {
        size_t leidos = 0;
        try {
            leidos = conexion->Read(buf, sizeof(buf));
        } catch (const std::exception&) {
            break;   // se cumplio el tiempo de espera
        }
        if (leidos == 0) break;
        acumulado.append(buf, leidos);
        conexion->SetTimeout(1);
    }
    size_t fin = acumulado.find('\n');
    if (fin != std::string::npos) acumulado.erase(fin);
    if (!acumulado.empty() && acumulado.back() == '\r') acumulado.pop_back();
    return acumulado;
}

// hilo por cada conexion tcp de otra isla: una solicitud y una respuesta, y se cierra
void atenderIsla(VSocket* conexion, TablaRutas* rutas, Reservas* reservas) {
    std::string ip = conexion->IpRemota();
    try {
        std::string solicitud = leerSolicitudIsla(conexion);
        if (solicitud.empty()) {
            std::cout << "[MAN TCP " << ip << "] no mando ninguna solicitud, se cierra\n";
        } else {
            std::cout << "[MAN TCP " << ip << " -> INT] " << solicitud << "\n";
            AtencionIsla atencion(*rutas, *reservas);
            std::string respuesta = atencion.responder(solicitud, ip);
            conexion->Write((respuesta + "\n").c_str());
            std::cout << "[MAN TCP INT -> " << ip << "] " << respuesta << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error atendiendo a la isla " << ip << ": " << e.what() << "\n";
    }
    conexion->Close();
    delete conexion;
}

// hilo que acepta las conexiones tcp de las otras islas en el puerto que se anuncia en el 60
void aceptarIslas(VSocket* servidorIslas, TablaRutas* rutas, Reservas* reservas) {
    for (;;) {
        try {
            VSocket* conexion = servidorIslas->AcceptConnection();
            std::thread trabajador(atenderIsla, conexion, rutas, reservas);
            trabajador.detach();
        } catch (const std::exception& e) {
            std::cerr << "error aceptando una isla: " << e.what() << "\n";
        }
    }
}

// hilo que revisa cada pocos segundos las reservas de otras islas que ya vencieron
// y le devuelve ese stock al servidor de productos con el mensaje interno 52
void vencerReservas(TablaRutas* rutas, Reservas* reservas) {
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(SEGUNDOS_ENTRE_VENCIMIENTOS));
        for (const auto& r : reservas->sacarVencidas()) {
            std::cout << "[INT] vencio la reserva de " << r.ipIsla << ": " << r.cantidad << " x " << r.producto
                      << ", se devuelve a " << r.servidor << "\n";
            devolverReserva(*rutas, r.servidor, r.producto, r.cantidad);
        }
    }
}

// lista separada por comas
static std::vector<std::string> separarComas(const std::string& texto) {
    std::vector<std::string> partes;
    std::stringstream ss(texto);
    std::string parte;
    while (std::getline(ss, parte, ',')) {
        if (!parte.empty()) partes.push_back(parte);
    }
    return partes;
}

static void mostrarUso(const char* programa) {
    std::cerr << "uso: " << programa << " <ip donde escucha a los clientes> <ip de la red de los servidores> [broadcast]\n"
              << "       [-isla NN] [-islas b1,b2,...] [-puerto N] [-http N]\n"
              << "  -isla    numero de isla para el mancomunado (por defecto 04)\n"
              << "  -islas   broadcasts donde se mandan el 60 y el 61 (por defecto los de las islas del laboratorio)\n"
              << "  -puerto  puerto tcp que se anuncia en el 60 (por defecto " << PUERTO_MANCOMUNADO << ")\n"
              << "  -http    puerto donde se atiende a los clientes (por defecto " << PUERTO_CLIENTES << ")\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        mostrarUso(argv[0]);
        return 1;
    }
    const char* ipClientes = argv[1];
    const char* ipPrivada = argv[2];
    if (!validarIp(ipClientes) || !validarIp(ipPrivada)) {
        std::cerr << "las ips deben ser ipv4\n";
        return 1;
    }

    // el broadcast de la isla sale de la ip de la red de los servidores (en la isla 4, 172.16.123.79)
    int siguiente = 3;
    std::string broadcast;
    if (argc > 3 && argv[3][0] != '-') broadcast = argv[siguiente++];
    else broadcast = broadcastDe(ipPrivada);
    if (broadcast.empty() || !validarIp(broadcast)) {
        std::cerr << "no se encontro la direccion de broadcast de " << ipPrivada << ", pasela como tercer argumento\n";
        return 1;
    }

    // datos para el mancomunado; las opciones son para probar varias islas en una sola computadora
    DatosIsla isla;
    isla.id = ID_INTERMEDIARIO;
    isla.ip = ipPrivada;
    isla.puerto = PUERTO_MANCOMUNADO;
    isla.broadcasts = BROADCASTS_ISLAS;
    int puertoClientes = PUERTO_CLIENTES;
    for (; siguiente < argc; siguiente++) {
        std::string opcion = argv[siguiente];
        if (siguiente + 1 >= argc) {
            mostrarUso(argv[0]);
            return 1;
        }
        std::string valor = argv[++siguiente];
        if (opcion == "-isla" && valor.size() == 2 && validarIdIsla("INT_" + valor)) {
            isla.id = "INT_" + valor;
        } else if (opcion == "-islas" && !separarComas(valor).empty()) {
            isla.broadcasts = separarComas(valor);
        } else if (opcion == "-puerto" && validarPuertoMancomunado(valor)) {
            isla.puerto = std::stoi(valor);
        } else if (opcion == "-http" && validarPuerto(valor)) {
            puertoClientes = std::stoi(valor);
        } else {
            mostrarUso(argv[0]);
            return 1;
        }
    }
    for (const auto& b : isla.broadcasts) {
        if (!validarIp(b)) {
            std::cerr << b << " no es una ip valida\n";
            return 1;
        }
    }

    // si un cliente cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    // sigint y sigterm solo los recibe esperarApagado (igual que en ServidorProductos.cc)
    sigset_t senales;
    sigemptyset(&senales);
    sigaddset(&senales, SIGINT);
    sigaddset(&senales, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &senales, nullptr);

    srand(time(nullptr) ^ getpid());
    siguienteIdMensaje = rand() % 10000;

    // las tres se comparten entre todos los hilos y cada una protege sus datos con su propio mutex
    TablaRutas rutas;
    TablaIslas islas;
    Reservas reservas;
    Carritos carritos;

    VSocket* servidor = new Socket('s');
    Socket* udp = new Socket('d');
    Socket* udpIslas = new Socket('d');
    VSocket* servidorIslas = new Socket('s');
    try {
        servidor->Bind(ipClientes, puertoClientes);
        servidor->MarkPassive(10);

        // el socket udp se liga a todas las interfaces: si se liga a la ip no le llegan los broadcast
        udp->Bind(PUERTO_UDP_INTERMEDIARIO);
        udp->HabilitarBroadcast();

        // otro socket udp para el mancomunado, tambien ligado a todas las interfaces
        udpIslas->Bind(PUERTO_MANCOMUNADO);
        udpIslas->HabilitarBroadcast();

        // las otras islas se conectan por la red privada, a la ip y el puerto del 60
        servidorIslas->Bind(ipPrivada, isla.puerto);
        servidorIslas->MarkPassive(10);
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el intermediario: " << e.what() << "\n";
        delete servidor;
        delete udp;
        delete udpIslas;
        delete servidorIslas;
        return 1;
    }

    std::cout << "intermediario escuchando en http://" << ipClientes << ":" << puertoClientes << "/\n";
    std::cout << "descubrimiento por udp en el puerto " << PUERTO_UDP_INTERMEDIARIO << ", broadcast " << broadcast << "\n";
    std::cout << "tabla de rutas en http://" << ipClientes << ":" << puertoClientes << "/rutas\n";
    std::cout << "mancomunado como " << isla.id << ": udp " << PUERTO_MANCOMUNADO << " para 60 y 61, tcp "
              << isla.ip << ":" << isla.puerto << " para 10, 20, 50 y 40\n";

    std::thread escucha(escucharServidores, udp, &rutas);
    escucha.detach();
    std::thread llamada(llamarServidores, udp, broadcast);
    llamada.detach();
    std::thread escuchaIslas(escucharIslas, udpIslas, &islas, isla);
    escuchaIslas.detach();
    std::thread anuncio(anunciarAlArrancar, udpIslas, isla);
    anuncio.detach();
    std::thread apagado(esperarApagado, udpIslas, isla, senales);
    apagado.detach();
    std::thread aceptaIslas(aceptarIslas, servidorIslas, &rutas, &reservas);
    aceptaIslas.detach();
    std::thread vencimientos(vencerReservas, &rutas, &reservas);
    vencimientos.detach();

    for (;;) {
        VSocket* cliente = servidor->AcceptConnection();
        std::thread trabajador(atenderCliente, cliente, &rutas, &islas, &carritos);
        trabajador.detach();
    }

    return 0;
}
