// Simulacion de TicAmazon con dos islas.
// Dentro de cada isla se usa el protocolo interno (ORIGEN|DESTINO/TIPO/campos):
// cliente con intermediario y intermediario con sus bodegas.
// Entre los intermediarios de las dos islas se usa el protocolo mancomunado (TIPO/campos).
// Todo pasa por un solo buzon; los canales hacen el papel de la red.

#include <thread>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bitacora.h"
#include "Bodega.h"
#include "Intermediario.h"
#include "Cliente.h"

int main() {
    Buzon buzon; // un solo buzon compartido por todos los hilos

    // isla 4 (equipo 4, cafeteria)
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

    // isla 5: otra isla de ejemplo con una sola bodega
    std::vector<Producto> productosBodega3 = {
        { "helados", "cono",   900.0f, 12, "cono de vainilla" },
        { "helados", "paleta", 600.0f, 30, "paleta de fresa" },
        { "snacks",  "papas",  700.0f, 20, "papas tostadas" }
    };

    Bodega bodega1( buzon, ID_BODEGA1, "Bodega1", ID_INT04, ID_TXT_INT04,
                     { "BebidasFrias", "BebidasCalientes", "Postres" }, productosBodega1 );
    Bodega bodega2( buzon, ID_BODEGA2, "Bodega2", ID_INT04, ID_TXT_INT04,
                     { "Caramelos", "Reposteria" }, productosBodega2 );
    Bodega bodega3( buzon, ID_BODEGA3, "Bodega3", ID_INT05, ID_TXT_INT05,
                     { "helados", "snacks" }, productosBodega3 );

    // los dos intermediarios reciben el broadcast de las islas
    std::vector<int> islas = { ID_INT04, ID_INT05 };
    Intermediario int04( buzon, ID_INT04, ID_TXT_INT04, "172.16.123.68", 2, islas );
    Intermediario int05( buzon, ID_INT05, ID_TXT_INT05, "172.16.123.84", 1, islas );
    Cliente cliente( buzon );

    titulo( "cada isla registra sus bodegas (protocolo interno)" );
    std::thread hBodega1( &Bodega::ejecutar, &bodega1 );
    std::thread hBodega2( &Bodega::ejecutar, &bodega2 );
    std::thread hBodega3( &Bodega::ejecutar, &bodega3 );
    int04.iniciar();
    int05.iniciar();

    titulo( "las islas se anuncian y piden categorias (protocolo mancomunado)" );
    int04.anunciarse();
    int05.anunciarse();
    int04.conocerIslas();
    int05.conocerIslas();

    titulo( "INT_05 consulta a la isla 4: casos normales y de error del mancomunado" );
    int05.probarIsla( ID_INT04 );

    titulo( "el cliente de la isla 4 compra en las dos islas" );
    std::thread hCliente( &Cliente::ejecutar, &cliente );
    int04.atenderCliente();
    hCliente.join();

    titulo( "cierre: despedidas por broadcast y fin de las bodegas" );
    int04.despedirse();
    int05.despedirse();
    int04.terminar();
    int05.terminar();

    hBodega1.join();
    hBodega2.join();
    hBodega3.join();

    return 0;
}
