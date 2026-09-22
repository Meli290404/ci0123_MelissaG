#include <iostream>
#include <thread>
#include <sstream>

#include "Socket.h"
#include "FileSystem.h"
#include "Bodega.h"
#include "Protocolo.h"
#include "Validaciones.h"
#include "Despachador.h"
#include "HttpUtil.h"

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

std::string construirRespuestaHTTP(const std::string& cuerpoHtml) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: text/html; charset=utf-8\r\n"
        << "Content-Length: " << cuerpoHtml.size() << "\r\n"
        << "Connection: close\r\n\r\n"
        << cuerpoHtml;
    return oss.str();
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

void atenderCliente(VSocket* cliente, Bodega* bodega1, Bodega* bodega2) {
    try {
        std::string peticion = leerPeticion(cliente);
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

        std::string respuestaHTTP = construirRespuestaHTTP(html);
        cliente->Write(respuestaHTTP.c_str());
        cliente->Close();
        delete cliente;

    } catch (const std::exception& e) {
        std::cerr << "error atendiendo cliente: " << e.what() << "\n";
        delete cliente;
    }
}

int main() {
    Filesystem fs("cafeteria.dat");
    fs.crearArchivo("TicAmazon");

    uint16_t bloqueBod1 = fs.registrarBodega("Bodega-1", "bod1");
    uint16_t bloqueBod2 = fs.registrarBodega("Bodega-2", "bod2");

    Bodega bodega1(fs, bloqueBod1);
    bodega1.crearCategoria("Bebidas frias", "bebfr");
    bodega1.crearCategoria("Bebidas calientes", "bebca");
    bodega1.crearCategoria("Postres", "postr");
    bodega1.agregarProducto("bebfr", "CafeFrio", 20, 150000);
    bodega1.agregarProducto("bebfr", "Limonada", 15, 100000);
    bodega1.agregarProducto("bebca", "CafeCaliente", 30, 140000);
    bodega1.agregarProducto("postr", "Tiramisu", 10, 250000);

    Bodega bodega2(fs, bloqueBod2);
    bodega2.crearCategoria("Caramelos", "caram");
    bodega2.crearCategoria("Reposteria", "repos");
    bodega2.agregarProducto("caram", "Chupeta", 50, 30000);
    bodega2.agregarProducto("repos", "PanDulce", 25, 80000);

    VSocket* servidor = new Socket('s');
    servidor->Bind(PUERTO_SERVIDOR);
    servidor->MarkPassive(10);

    std::cout << "ServidorTicAmazon escuchando en el puerto " << PUERTO_SERVIDOR << "\n";

    for (;;) {
        VSocket* cliente = servidor->AcceptConnection();
        std::thread trabajador(atenderCliente, cliente, &bodega1, &bodega2);
        trabajador.detach();
    }

    return 0;
}