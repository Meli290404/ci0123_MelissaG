#include <iostream>
#include <thread>
#include <csignal>
#include <chrono>
#include <mutex>
#include <atomic>

#include "Socket.h"
#include "FileSystem.h"
#include "AdministradorBodegas.h"
#include "AtencionBodega.h"
#include "Protocolo.h"
#include "Validaciones.h"
#include "Descubrimiento.h"

static const char* RUTA_ARCHIVO = "fs.dat.ci0123";
static const int MAXLINE = 512;
static const int INTENTOS_BUSCAR_INTERMEDIARIO = 3;
static const int SEGUNDOS_ENTRE_INTENTOS = 2;

// datos que necesita el hilo de descubrimiento para armar los mensajes 80 y 83
struct DatosServidor {
    std::string id;           // SERV_NN
    int puertoTcp;            // 9050 + NN
    std::string broadcast;    // 172.16.123.79 en la isla 4
    Filesystem* fs;
};

// el hilo de descubrimiento y los hilos que atienden 10, 20 y 50 usan el mismo fs
static std::mutex mutexFs;

// lo pone el hilo de escucha cuando llega el 81; con eso se deja de mandar el 80
static std::atomic<bool> registrado(false);

// lee hasta el '\n' que cierra el mensaje; una linea que pasa de MAX_LINEA sin '\n' se descarta
std::string leerMensaje(VSocket* conexion) {
    char buf[MAXLINE];
    std::string acumulado;
    while (acumulado.find('\n') == std::string::npos && acumulado.size() <= MAX_LINEA) {
        size_t leidos = conexion->Read(buf, sizeof(buf));
        if (leidos == 0) break;
        acumulado.append(buf, leidos);
    }

    size_t fin = acumulado.find('\n');
    if (fin == std::string::npos && acumulado.size() > MAX_LINEA) return "";
    if (fin != std::string::npos) acumulado.erase(fin);
    if (!acumulado.empty() && acumulado.back() == '\r') acumulado.pop_back();
    return acumulado;
}

void atenderIntermediario(VSocket* conexion, Filesystem* fs, std::string idServidor) {
    try {
        std::string mensaje = leerMensaje(conexion);
        if (!mensaje.empty()) {
            std::cout << "[INT -> SERV] " << mensaje << "\n";

            std::string respuesta;
            {
                std::lock_guard<std::mutex> lock(mutexFs);
                AdministradorBodegas admin(*fs);
                AtencionBodega atencion(admin, idServidor);
                respuesta = atencion.responder(mensaje);
            }

            std::cout << "[SERV -> INT] " << respuesta << "\n";
            respuesta += "\n";
            conexion->Write(respuesta.data(), respuesta.size());
        }
        conexion->Close();
    } catch (const std::exception& e) {
        std::cerr << "error atendiendo al intermediario: " << e.what() << "\n";
    }
    delete conexion;
}

// categorias de todas las bodegas del archivo, sin repetir, para anunciarlas en el 80 y el 83
static std::vector<std::string> categoriasDelArchivo(Filesystem* fs) {
    std::lock_guard<std::mutex> lock(mutexFs);
    AdministradorBodegas admin(*fs);
    return admin.listarCategorias();
}

static std::string mensajeAnuncio(const DatosServidor& datos, const std::string& destino, int tipo) {
    return construirMensaje(datos.id, destino, tipo, camposAnuncio(datos.puertoTcp, categoriasDelArchivo(datos.fs)));
}

// hilo que espera datagramas con recvFrom en el puerto udp de los servidores:
//   81: el intermediario confirma que recibio nuestro 80
//   82: el intermediario busca servidores, se le contesta 83 directo a su ip
void escucharIntermediario(Socket* udp, DatosServidor datos) {
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
        // el 80 que mandamos por broadcast tambien nos llega a nosotros y a los otros servidores
        if (msg.tipo == TIPO_BUSCO_INTERMEDIARIO) continue;
        if (msg.destino != datos.id && msg.destino != ID_BROADCAST) continue;

        std::cout << "[UDP " << ip << " -> " << datos.id << "] " << texto << "\n";
        if (msg.tipo == TIPO_SOY_INTERMEDIARIO) {
            std::cout << "[" << datos.id << "] el intermediario " << msg.origen << " en " << ip
                      << " nos registro en su tabla de rutas\n";
            registrado = true;
        } else if (msg.tipo == TIPO_BUSCO_SERVIDORES) {
            std::string respuesta = mensajeAnuncio(datos, msg.origen, TIPO_SOY_SERVIDOR);
            try {
                enviarDatagrama(*udp, respuesta, ip, PUERTO_UDP_INTERMEDIARIO);
                std::cout << "[UDP " << datos.id << " -> " << ip << "] " << respuesta << "\n";
            } catch (const std::exception& e) {
                std::cerr << "no se pudo contestar el 82: " << e.what() << "\n";
            }
        }
    }
}

// al arrancar el servidor no sabe donde esta el intermediario: manda el 80 por broadcast.
// se repite hasta que llegue el 81 o se acaben los intentos; si el intermediario todavia no esta
// corriendo, igual lo va a encontrar despues con su 82
void buscarIntermediario(Socket* udp, DatosServidor datos) {
    for (int intento = 1; intento <= INTENTOS_BUSCAR_INTERMEDIARIO && !registrado; intento++) {
        std::string mensaje = mensajeAnuncio(datos, ID_BROADCAST, TIPO_BUSCO_INTERMEDIARIO);
        try {
            enviarDatagrama(*udp, mensaje, datos.broadcast, PUERTO_UDP_INTERMEDIARIO);
            std::cout << "[UDP " << datos.id << " -> " << datos.broadcast << "] " << mensaje << "\n";
        } catch (const std::exception& e) {
            std::cerr << "no se pudo mandar el 80: " << e.what() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(SEGUNDOS_ENTRE_INTENTOS));
    }
    if (!registrado) {
        std::cout << "[" << datos.id << "] ningun intermediario contesto el 80, se espera su 82\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "uso: " << argv[0] << " <numero de servidor> <ip donde escucha> [archivo] [broadcast]\n";
        return 1;
    }
    if (!validarNumeroServidor(argv[1]) || !validarIp(argv[2])) {
        std::cerr << "el numero de servidor va de 1 a 99 y la ip debe ser ipv4\n";
        return 1;
    }
    int numero = std::stoi(argv[1]);
    const char* ipServidor = argv[2];
    const char* rutaArchivo = argc >= 4 ? argv[3] : RUTA_ARCHIVO;

    DatosServidor datos;
    datos.id = idServidor(numero);
    datos.puertoTcp = puertoServidor(numero);
    datos.broadcast = argc >= 5 ? argv[4] : broadcastDe(ipServidor);
    if (datos.broadcast.empty() || !validarIp(datos.broadcast)) {
        std::cerr << "no se encontro la direccion de broadcast de " << ipServidor << ", pasela como quinto argumento\n";
        return 1;
    }

    // si el intermediario cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    // el archivo lo crea cargarInventario.sh (o adminBodegas); sin el no hay nada que anunciar
    Filesystem fs(rutaArchivo);
    if (!fs.archivoValido()) {
        std::cerr << rutaArchivo << " no existe o no es valido, primero corra ./cargarInventario.sh " << rutaArchivo << "\n";
        return 1;
    }
    datos.fs = &fs;

    VSocket* servidor = new Socket('s');
    Socket* udp = new Socket('d');
    try {
        servidor->Bind(ipServidor, datos.puertoTcp);
        servidor->MarkPassive(10);

        // el socket udp se liga a todas las interfaces: si se liga a la ip no le llegan los broadcast
        udp->Bind(PUERTO_UDP_SERVIDORES);
        udp->HabilitarBroadcast();
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el servidor de productos: " << e.what() << "\n";
        delete servidor;
        delete udp;
        return 1;
    }

    std::cout << datos.id << " con " << rutaArchivo << " escuchando en " << ipServidor << ":" << datos.puertoTcp
              << " (tcp) y en el puerto " << PUERTO_UDP_SERVIDORES << " (udp), broadcast " << datos.broadcast << "\n";

    std::thread escucha(escucharIntermediario, udp, datos);
    escucha.detach();
    std::thread busqueda(buscarIntermediario, udp, datos);
    busqueda.detach();

    for (;;) {
        VSocket* conexion = servidor->AcceptConnection();
        std::thread trabajador(atenderIntermediario, conexion, &fs, datos.id);
        trabajador.detach();
    }

    return 0;
}
