#ifndef INTERMEDIARIO_H
#define INTERMEDIARIO_H

#include <map>
#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"

class Intermediario {
public:
    explicit Intermediario( Buzon & buzon );

    void ejecutar();

private:
    Buzon & buzon_;

    std::map<std::string, int> categoriaBodega_; // categoria -> id numerico de la bodega dueña
    std::map<int, std::string> nombreBodega_;    // id numerico -> nombre (para los rotulos)
    std::vector<ItemCarrito> carrito_;

    void registrarBodegas();
    void manejarReCat( const std::string & categoria );
    void manejarReProd( const std::string & nombre );
    void manejarAddCart( const std::string & producto, const std::string & countTexto );
    void manejarRequestFactura( const std::string & countTexto );

    void enviarErrFormat( const std::string & tipoOriginal, const std::string & campo, const std::string & valor );
    void enviarErrComm( const std::string & tipoPendiente, int intentos );
};

#endif