#ifndef CONEXIONISLA_H
#define CONEXIONISLA_H

#include <string>
#include <vector>
#include "Mancomunado.h"
#include "TablaIslas.h"

// envia mensajes del protocolo mancomunado al intermediario de otra isla por tcp.
// igual que ConexionBodega: cada mensaje usa su propia conexion, se manda una linea y se lee la respuesta
class ConexionIsla {
public:
    // timeout de 5 s y hasta 3 intentos, como recomienda el protocolo mancomunado
    static const int INTENTOS = 3;
    static const int SEGUNDOS_ESPERA = 5;

    ConexionIsla(const std::string& ip, int puerto);

    // devuelve false si ninguno de los intentos tuvo respuesta; intentos queda con los que se hicieron
    bool enviar(const std::string& mensaje, std::string& respuesta, int& intentos);

private:
    std::string ip_;
    int puerto_;

    bool intentarUnaVez(const std::string& mensaje, std::string& respuesta);
};

// envia un mensaje a otra isla y deja la respuesta ya parseada. solo acepta 01, 02 y 03.
// si la isla no contesta, se marca sin respuesta en la tabla de islas hasta su proximo 60
bool consultarIsla(TablaIslas& islas, const IslaRuta& isla, int tipo, const std::vector<std::string>& campos,
                   MensajeMancomunado& respuesta);

// le pide las categorias a otra isla (10) y las guarda en la tabla de islas. false si no contesto
// o si la respuesta no es una lista valida
bool pedirCategoriasIsla(TablaIslas& islas, const IslaRuta& isla, std::vector<std::string>& categorias);

// le pide los productos de una categoria a otra isla (20) y los guarda en la tabla de islas.
// existe queda en false si la isla contesto 03. devuelve false si no contesto o la lista no es valida
bool pedirProductosIsla(TablaIslas& islas, const IslaRuta& isla, const std::string& categoria,
                        std::vector<ProductoDeIsla>& productos, bool& existe);

#endif
