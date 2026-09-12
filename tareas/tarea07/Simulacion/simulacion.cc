// Simulacion de TicAmazon (Equipo 4 - cafeteria) con el protocolo nuevo
// (ORIGEN|DESTINO/TIPO/campos) y dos bodegas.

#include <thread>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bodega.h"
#include "Intermediario.h"
#include "Cliente.h"

int main() {
    Buzon buzon; // un solo buzon compartido por todos los hilos

    std::vector<Producto> productosBodega1 = {
        { "BebidasCalientes", "cafe",      1000.0f, 50, "cafe negro" },
        { "BebidasCalientes", "capuchino", 1200.0f, 40, "espresso con leche" },
        { "Postres",          "tarta",     2200.0f, 10, "tarta de queso del dia" },
        { "Postres",          "flan",      1500.0f,  8, "flan de vainilla casero" }
    };

    std::vector<Producto> productosBodega2 = {
        { "Caramelos",   "menta",      300.0f, 100, "caramelo de menta individual" },
        { "Caramelos",   "fresa",      350.0f,  80, "caramelo de fresa individual" },
        { "Reposteria",  "pan-dulce",  900.0f,  20, "pan dulce relleno de queso" },
        { "Reposteria",  "galleta",    250.0f,  60, "galleta de mantequilla" }
    };

    Bodega bodega1( buzon, ID_BODEGA1, "Bodega1",
                     { "BebidasFrias", "BebidasCalientes", "Postres" }, productosBodega1 );
    Bodega bodega2( buzon, ID_BODEGA2, "Bodega2",
                     { "Caramelos", "Reposteria" }, productosBodega2 );
    Intermediario intermediario( buzon );
    Cliente cliente( buzon );

    std::thread hBodega1( &Bodega::ejecutar, &bodega1 );
    std::thread hBodega2( &Bodega::ejecutar, &bodega2 );
    std::thread hIntermediario( &Intermediario::ejecutar, &intermediario );
    std::thread hCliente( &Cliente::ejecutar, &cliente );

    hCliente.join();
    hIntermediario.join();
    hBodega1.join();
    hBodega2.join();

    return 0;
}