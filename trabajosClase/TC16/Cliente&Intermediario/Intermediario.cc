#include <iostream>
#include <thread>
#include <csignal>

#include "Socket.h"
#include "HttpUtil.h"
#include "ConexionBodega.h"
#include "Carritos.h"
#include "AtencionCliente.h"

static const int PUERTO_CLIENTES = 8080;
static const int PUERTO_SERVIDOR_PRODUCTOS = 9091;   // el mismo PUERTO_BODEGA de ServidorProductos.cc
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

void atenderCliente(VSocket* cliente, ConexionBodega* bodega, Carritos* carritos) {
    try {
        std::string peticion = leerPeticion(cliente);

        // algunos navegadores abren conexiones de reserva y las cierran sin enviar nada
        if (!peticion.empty()) {
            std::cout << "[CLI -> INT] " << peticion.substr(0, peticion.find("\r\n")) << "\n";

            AtencionCliente atencion(*bodega, *carritos);
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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "uso: " << argv[0] << " <ip donde escucha a los clientes> <ip del servidor de productos>\n";
        return 1;
    }
    const char* ipClientes = argv[1];
    const char* ipServidorProductos = argv[2];

    // si un cliente cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    // los dos se comparten entre todos los hilos: ConexionBodega no guarda estado
    // y Carritos protege sus datos con su propio mutex
    ConexionBodega bodega(ipServidorProductos, PUERTO_SERVIDOR_PRODUCTOS);
    Carritos carritos;

    VSocket* servidor = new Socket('s');
    try {
        servidor->Bind(ipClientes, PUERTO_CLIENTES);
        servidor->MarkPassive(10);
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el intermediario: " << e.what() << "\n";
        delete servidor;
        return 1;
    }

    std::cout << "intermediario escuchando en http://" << ipClientes << ":" << PUERTO_CLIENTES << "/\n";
    std::cout << "servidor de productos en " << ipServidorProductos << ":" << PUERTO_SERVIDOR_PRODUCTOS << "\n";

    for (;;) {
        VSocket* cliente = servidor->AcceptConnection();
        std::thread trabajador(atenderCliente, cliente, &bodega, &carritos);
        trabajador.detach();
    }

    return 0;
}