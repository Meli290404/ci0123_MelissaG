#ifndef CLIENTE_H
#define CLIENTE_H

#include <string>
#include <vector>

#include "Buzon.h"

class Cliente {
public:
    explicit Cliente( Buzon & buzon );
    void ejecutar();

private:
    Buzon & buzon_;
    void pedir( int tipo, const std::vector<std::string> & campos );
};

#endif