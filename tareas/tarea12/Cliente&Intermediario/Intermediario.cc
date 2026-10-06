#include <iostream>
#include <thread>
#include <sstream>
#include <csignal>

#include "Socket.h"
#include "FileSystem.h"
#include "Bodega.h"
#include "AdministradorBodegas.h"
#include "PaginaWeb.h"
#include "Protocolo.h"
#include "Validaciones.h"
#include "Despachador.h"
#include "HttpUtil.h"

static const char* RUTA_ARCHIVO = "cafeteria.dat";
static const int PUERTO_SERVIDOR = 8080;
static const int MAXLINE = 4096;

std::string leerPeticion(VSocket* client) {
    char buf[MAXLINE];
    std::string acumulado;
    while (acumulado.find("\r\n\r\n") == std::string::npos) {
        int leidos = client->Read(buf, MAXLINE - 1);
        if (leidos <= 0) break;
        acumulado.append(buf, leidos);
    }
    return acumulado;
}

std::string extraerQueryMsg(const std::string& peticion) {
    size_t inicio = peticion.find("GET ");
    if (inicio == std::string::npos) return "";
    inicio += 4;
    size_t fin = peticion.find(" HTTP", inicio);
    if (fin == std::string::npos) return "";
    std::string url = peticion.substr(inicio, fin - inicio);
    size_t signo = url.find("msg=");
    if (signo == std::string::npos) return "";
    return urlDecode(url.substr(signo + 4));
}

std::string construirHtmlErrorFormato(int tipoOriginal, const std::string& campo, const std::string& valor) {
    return "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_ERR_FORMATO) +
           "\" data-tipo-original=\"" + std::to_string(tipoOriginal) + "\" data-campo=\"" + campo +
           "\" data-valor=\"" + valor + "\"><p>Error de formato en el campo " + campo + "</p></div>";
}

std::string construirHtmlErrorTamano(const std::string& campo, size_t tamRecibido, size_t tamMax) {
    return "<div class=\"respuesta\" data-tipo=\"" + std::to_string(TIPO_ERR_TAMANO) +
           "\" data-campo=\"" + campo + "\" data-tam-recibido=\"" + std::to_string(tamRecibido) +
           "\" data-tam-max=\"" + std::to_string(tamMax) + "\"><p>Mensaje excede el tamano maximo</p></div>";
}

// atiende /tic?msg=..., la ruta del protocolo que usa Cliente.cc, y devuelve el html de la respuesta
std::string atenderProtocolo(const std::string& peticion, Bodega* bodega1, Bodega* bodega2) {
    std::string mensajeTexto = extraerQueryMsg(peticion);
    std::string html;
    int tipoResp;

    if (mensajeTexto.empty()) {
        html = construirHtmlErrorFormato(0, "mensaje", "vacio");
        tipoResp = TIPO_ERR_FORMATO;
    } else if (!validarTamanoMensaje(mensajeTexto)) {
        html = construirHtmlErrorTamano("mensaje", mensajeTexto.size(), TAM_MAX_MENSAJE);
        tipoResp = TIPO_ERR_TAMANO;
    } else {
        try {
            Mensaje msg = parsearMensaje(mensajeTexto);
            std::cout << "[recibido] " << mensajeTexto << "\n";

            std::string campoInvalido, valorInvalido;
            if (!validarMensaje(msg, campoInvalido, valorInvalido)) {
                html = construirHtmlErrorFormato(msg.tipo, campoInvalido, valorInvalido);
                tipoResp = TIPO_ERR_FORMATO;
            } else {
                Despachador despachador(*bodega1, *bodega2);
                despachador.procesar(msg, tipoResp, html);
            }
        } catch (const std::exception&) {
            html = construirHtmlErrorFormato(0, "mensaje", mensajeTexto);
            tipoResp = TIPO_ERR_FORMATO;
        }
    }

    std::cout << "[respondido] tipo " << tipoResp << "\n";
    return html;
}

void atenderCliente(VSocket* cliente, Filesystem* fs, Bodega* bodega1, Bodega* bodega2) {
    try {
        std::string peticion = leerPeticion(cliente);

        // algunos navegadores abren conexiones de reserva y las cierran sin enviar nada
        if (peticion.empty()) {
            delete cliente;
            return;
        }

        PaginaWeb pagina;
        std::string ruta = extraerRuta(peticion);
        std::string respuestaHTTP;

        if (peticion.rfind("GET ", 0) != 0) {
            respuestaHTTP = construirRespuestaHttp(405, pagina.error(405, "Este servidor solo acepta peticiones GET."));
        } else if (ruta == "/") {
            // la pagina se arma leyendo el archivo en cada peticion
            AdministradorBodegas admin(*fs);
            respuestaHTTP = construirRespuestaHttp(200, pagina.inventario(admin.leerInventario()));
        } else if (ruta == "/tic") {
            respuestaHTTP = construirRespuestaHttp(200, atenderProtocolo(peticion, bodega1, bodega2));
        } else {
            respuestaHTTP = construirRespuestaHttp(404, pagina.error(404, "La página " + ruta + " no existe."));
        }

        std::cout << "[peticion] " << ruta << "\n";
        cliente->Write(respuestaHTTP.c_str());
        cliente->Close();
        delete cliente;

    } catch (const std::exception& e) {
        std::cerr << "error atendiendo cliente: " << e.what() << "\n";
        delete cliente;
    }
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
        std::cerr << "uso: " << argv[0] << " <ip del laboratorio 3-5>\n";
        return 1;
    }
    const char* ipServidor = argv[1];

    // si un navegador cierra la conexion antes de tiempo, write no debe terminar todo el proceso
    signal(SIGPIPE, SIG_IGN);

    Filesystem fs(RUTA_ARCHIVO);
    if (!fs.archivoValido()) {
        std::cout << RUTA_ARCHIVO << " no existe o no es valido, se crea con los datos iniciales\n";
        fs.crearArchivo("TicAmazon");
        cargarDatosIniciales(fs);
    }

    // el despachador del protocolo todavia trabaja con estas dos bodegas fijas
    DirBodega dirBod1, dirBod2;
    if (!fs.buscarBodega("bod1", dirBod1) || !fs.buscarBodega("bod2", dirBod2)) {
        std::cerr << "no se encontraron bod1 y bod2 en " << RUTA_ARCHIVO << "\n";
        return 1;
    }
    Bodega bodega1(fs, dirBod1.bloqueDirCategorias);
    Bodega bodega2(fs, dirBod2.bloqueDirCategorias);

    VSocket* servidor = new Socket('s');
    try {
        servidor->Bind(ipServidor, PUERTO_SERVIDOR);
        servidor->MarkPassive(10);
    } catch (const std::exception& e) {
        std::cerr << "no se pudo abrir el servidor: " << e.what() << "\n";
        delete servidor;
        return 1;
    }

    std::cout << "ServidorTicAmazon escuchando en http://" << ipServidor << ":" << PUERTO_SERVIDOR << "/\n";

    for (;;) {
        VSocket* cliente = servidor->AcceptConnection();
        std::thread trabajador(atenderCliente, cliente, &fs, &bodega1, &bodega2);
        trabajador.detach();
    }

    return 0;
}