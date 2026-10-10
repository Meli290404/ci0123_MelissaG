#include <iostream>
#include <thread>
#include <csignal>

#include "Socket.h"
#include "FileSystem.h"
#include "AdministradorBodegas.h"
#include "AtencionBodega.h"
#include "Protocolo.h"

static const char* RUTA_ARCHIVO = "cafeteria.dat";
static const int PUERTO_BODEGA = 9091;
static const int MAXLINE = 512;

// lee hasta el '\n' que cierra el mensaje; deja de leer si pasa del doble de TAM_MAX_MENSAJE
std::string leerMensaje(VSocket* conexion) {
    char buf[MAXLINE];
    std::string acumulado;
    while (acumulado.find('\n') == std::string::npos && acumulado.size() <= 2 * TAM_MAX_MENSAJE) {
        size_t leidos = conexion->Read(buf, sizeof(buf));
        if (leidos == 0) break;
        acumulado.append(buf, leidos);
    }

    size_t fin = acumulado.find('\n');
    if (fin != std::string::npos) acumulado.erase(fin);
    if (!acumulado.empty() && acumulado.back() == '\r') acumulado.pop_back();
    return acumulado;
}

void atenderIntermediario(VSocket* conexion, Filesystem* fs) {
    try {
        std::string mensaje = leerMensaje(conexion);
        if (!mensaje.empty()) {
            std::cout << "[INT -> SERV] " << mensaje << "\n";

            AdministradorBodegas admin(*fs);
            AtencionBodega atencion(admin);
            std::string respuesta = atencion.responder(mensaje);

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

static void revisar(bool ok, const std::string& error) {
    if (!ok) std::cerr << "carga inicial: " << error << "\n";
}

// solo se llama cuando el archivo no existe o no trae la firma
void cargarDatosIniciales(Filesystem& fs) {
    AdministradorBodegas admin(fs);
    std::string error;

    revisar(admin.agregarBodega("bod1", "Bodega-1", error), error);
    revisar(admin.agregarCategoria("bod1", "bebfr", "Bebidas frias", error), error);
    revisar(admin.agregarCategoria("bod1", "bebca", "Bebida caliente", error), error);
    revisar(admin.agregarCategoria("bod1", "postr", "Postres", error), error);
    revisar(admin.agregarProducto("bod1", "bebfr", "CafeFrio", 20, 150000, error), error);
    revisar(admin.agregarProducto("bod1", "bebfr", "Limonada", 15, 100000, error), error);
    revisar(admin.agregarProducto("bod1", "bebca", "CafeCaliente", 30, 140000, error), error);
    revisar(admin.agregarProducto("bod1", "postr", "Tiramisu", 10, 250000, error), error);

    revisar(admin.agregarBodega("bod2", "Bodega-2", error), error);
    revisar(admin.agregarCategoria("bod2", "caram", "Caramelos", error), error);
    revisar(admin.agregarCategoria("bod2", "repos", "Reposteria", error), error);
    revisar(admin.agregarProducto("bod2", "caram", "Chupeta", 50, 30000, error), error);
    revisar(admin.agregarProducto("bod2", "repos", "PanDulce", 25, 80000, error), error);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "uso: " << argv[0] << " <ip donde escucha el servidor de productos>\n";
        return 1;
    }
    const char* ipServidor = argv[1];

    // si el intermediario cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    Filesystem fs(RUTA_ARCHIVO);
    if (!fs.archivoValido()) {
        std::cout << RUTA_ARCHIVO << " no existe o no es valido, se crea con los datos iniciales\n";
        fs.crearArchivo("TicAmazon");
        cargarDatosIniciales(fs);
    }

    VSocket* servidor = new Socket('s');
    try {
        servidor->Bind(ipServidor, PUERTO_BODEGA);
        servidor->MarkPassive(10);
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el servidor de productos: " << e.what() << "\n";
        delete servidor;
        return 1;
    }

    std::cout << "servidor de productos escuchando en " << ipServidor << ":" << PUERTO_BODEGA << "\n";

    for (;;) {
        VSocket* conexion = servidor->AcceptConnection();
        std::thread trabajador(atenderIntermediario, conexion, &fs);
        trabajador.detach();
    }

    return 0;
}