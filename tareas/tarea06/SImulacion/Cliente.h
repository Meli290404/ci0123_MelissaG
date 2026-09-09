#ifndef CLIENTE_H
#define CLIENTE_H

#include "Buzon.h"

// Esta clase es solo un guion de pruebas para probar el protocolo
class Cliente {
public:
    explicit Cliente( Buzon & buzon );
    void ejecutar();

private:
    Buzon & buzon_;
    void pedir( const std::string & peticion );
};

#endif