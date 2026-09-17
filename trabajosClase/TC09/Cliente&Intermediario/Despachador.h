#ifndef DESPACHADOR_H
#define DESPACHADOR_H

#include <string>
#include "Protocolo.h"
#include "Bodega.h"

// valida los campos de un mensaje ya parseado segun su tipo
bool validarMensaje(const Mensaje& msg, std::string& campoInvalido, std::string& valorInvalido);

std::string formatearPrecio(uint32_t centavos);

// conecta los mensajes del protocolo con Filesystem/Bodega. como el
// servidor es integrado, esta clase ES la logica de SERV, llamada directo
// en vez de mandarse mensajes por un socket aparte
class Despachador {
public:
    Despachador(Bodega& bodega1, Bodega& bodega2);

    void procesar(const Mensaje& msg, int& tipoResp, std::string& html);

private:
    Bodega& bodega1_;
    Bodega& bodega2_;

    Bodega* encontrarBodegaConCategoria(const std::string& idCategoria, DirCategoria& catOut);
    bool buscarProductoPorNombre(const std::string& nombre, RegistroProducto& prodOut);

    void manejarRequestCategory(int& tipoResp, std::string& html);
    void manejarRequestProducts(const std::string& idCategoria, int& tipoResp, std::string& html);
    void manejarRequestDetail(const std::string& nombreProducto, int& tipoResp, std::string& html);
    void manejarAddToCart(const std::string& producto, const std::string& countStr, int& tipoResp, std::string& html);
    void manejarFactura(const std::vector<std::string>& campos, int& tipoResp, std::string& html);
};

#endif