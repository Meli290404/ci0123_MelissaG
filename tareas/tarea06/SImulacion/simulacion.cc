// Simulación de la primera etapa de TicAmazon (Equipo 4 - cafeteria).
// Tres hilos: Cliente, Intermediario y Bodega

#include <thread>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bodega.h"
#include "Intermediario.h"
#include "Cliente.h"

int main() {
    Buzon buzon; // un solo buzon compartido por los tres hilos

    // Categorias dentro de la bodega
    std::vector<Producto> productosBodega = {
        { "cafe",      1000.0f, 50, "cafe negro" },
        { "capuchino", 1200.0f, 40, "espresso con leche" },
        { "tarta",     2200.0f, 10, "tarta de queso del dia" },
        { "flan",      1500.0f,  8, "flan de vainilla casero" },
        { "menta",      300.0f, 100, "caramelo de menta individual" },
        { "fresa",      350.0f,  80, "caramelo de fresa individual" },
        { "pan-dulce",  900.0f,  20, "pan dulce relleno de queso" },
        { "galleta",    250.0f,  60, "galleta de mantequilla" }
    };

    Bodega bodega( buzon, ID_BODEGA, "Bodega",
                   { "Bebidas", "Postres", "Caramelos", "Reposteria" }, productosBodega );
    Intermediario intermediario( buzon );
    Cliente cliente( buzon );

    std::thread hBodega( &Bodega::ejecutar, &bodega );
    std::thread hIntermediario( &Intermediario::ejecutar, &intermediario );
    std::thread hCliente( &Cliente::ejecutar, &cliente );

    // El cliente manda EXIT a intermediario; este lo reenvía a la bodega antes de terminar él mismo. Por eso join() en
    // los tres hilos no se queda bloqueado para siempre.
    hCliente.join();
    hIntermediario.join();
    hBodega.join();

    return 0;
}