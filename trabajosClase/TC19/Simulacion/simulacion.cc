// Simulacion de TicAmazon con dos islas.
// Dentro de cada isla se usa el protocolo interno (ORIGEN|DESTINO/TIPO/campos):
// cliente con intermediario y intermediario con sus bodegas.
// Entre los intermediarios de las dos islas se usa el protocolo mancomunado (TIPO/campos).
// Todo pasa por un solo buzon; los canales hacen el papel de la red.
// Cada isla guarda sus bodegas en su propio filesystem (un solo archivo): las bodegas
// leen y escriben sus productos ahi en cada mensaje 10, 20 y 50.

#include <thread>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bitacora.h"
#include "Bodega.h"
#include "Intermediario.h"
#include "Cliente.h"
#include "FileSystem.h"
#include "BodegaFS.h"
#include "AdministradorBodegas.h"

static const char * ARCHIVO_ISLA4 = "fs.dat.ci0123";
static const char * ARCHIVO_ISLA5 = "fs5.dat.ci0123";

struct BodegaInicial {
    std::string nombre;
    std::vector<Producto> productos;
};

// formatea el archivo y guarda las bodegas con sus productos, una linea por producto
static bool crearIsla( Filesystem & fs, const std::string & ruta, const std::string & negocio,
                       const std::vector<BodegaInicial> & bodegas ) {
    fs.crearArchivo( negocio );
    AdministradorBodegas admin( fs );
    std::string error;
    for ( const auto & bodega : bodegas ) {
        if ( !admin.agregarBodega( bodega.nombre, error ) ) {
            std::cerr << ruta << ": " << error << "\n";
            return false;
        }
        for ( const auto & p : bodega.productos ) {
            uint32_t centavos = (uint32_t) std::lround( p.precio * 100 );
            if ( !admin.agregarProducto( bodega.nombre, p.categoria, p.nombre, p.stock, centavos, error ) ) {
                std::cerr << ruta << ": " << error << "\n";
                return false;
            }
        }
    }
    printf( "    %s creado: %zu bodegas, %d bloques libres de %d\n", ruta.c_str(), bodegas.size(),
            fs.bloquesLibres(), NUM_BLOQUES_CAFETERIA );
    return true;
}

// si se pidio conservar y el archivo es valido se usa tal cual; si no, se crea de nuevo
static bool prepararIsla( Filesystem & fs, const std::string & ruta, const std::string & negocio,
                          const std::vector<BodegaInicial> & bodegas, bool conservar ) {
    if ( conservar && fs.archivoValido() ) {
        printf( "    %s ya existe, se usa con el stock que tenia (%d bloques libres)\n",
                ruta.c_str(), fs.bloquesLibres() );
        return true;
    }
    return crearIsla( fs, ruta, negocio, bodegas );
}

// abre el archivo otra vez, con un Filesystem nuevo, y muestra cada bodega tal como quedo en disco
static void mostrarDisco( const std::string & ruta ) {
    Filesystem fs( ruta );
    if ( !fs.archivoValido() ) {
        printf( "    %s no es valido\n", ruta.c_str() );
        return;
    }
    printf( "    %s (%d bloques libres de %d)\n", ruta.c_str(), fs.bloquesLibres(), NUM_BLOQUES_CAFETERIA );
    for ( const auto & entrada : fs.listarBodegas() ) {
        std::string nombre( entrada.nombre, strnlen( entrada.nombre, sizeof( entrada.nombre ) ) );
        printf( "      %s (i-nodo %u)\n", nombre.c_str(), (unsigned) entrada.inodo );
        BodegaFS bodega( fs, entrada.inodo, nombre );
        for ( const auto & linea : bodega.leerLineas() ) {
            printf( "        %s", lineaATexto( linea ).c_str() );
        }
    }
}

int main( int argc, char * argv[] ) {
    // -c conserva los archivos de una corrida anterior; sin el, cada corrida empieza con el stock inicial
    bool conservar = argc > 1 && std::string( argv[ 1 ] ) == "-c";

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

    // un filesystem por isla; las bodegas de la misma isla comparten el archivo y su mutex
    titulo( "cada isla prepara su almacenamiento (un archivo con varias bodegas)" );
    Filesystem fsIsla4( ARCHIVO_ISLA4 );
    Filesystem fsIsla5( ARCHIVO_ISLA5 );
    if ( !prepararIsla( fsIsla4, ARCHIVO_ISLA4, "TicAmazon isla 4",
                        { { "Bodega1", productosBodega1 }, { "Bodega2", productosBodega2 } }, conservar ) ||
         !prepararIsla( fsIsla5, ARCHIVO_ISLA5, "TicAmazon isla 5",
                        { { "Bodega3", productosBodega3 } }, conservar ) ) {
        return 1;
    }

    // cada bodega busca su nombre en el directorio del archivo para saber cual es su i-nodo
    Bodega bodega1( buzon, ID_BODEGA1, "Bodega1", ID_INT04, ID_TXT_INT04, fsIsla4, ARCHIVO_ISLA4 );
    Bodega bodega2( buzon, ID_BODEGA2, "Bodega2", ID_INT04, ID_TXT_INT04, fsIsla4, ARCHIVO_ISLA4 );
    Bodega bodega3( buzon, ID_BODEGA3, "Bodega3", ID_INT05, ID_TXT_INT05, fsIsla5, ARCHIVO_ISLA5 );

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

    titulo( "estado final del almacenamiento, leido otra vez desde el disco" );
    mostrarDisco( ARCHIVO_ISLA4 );
    mostrarDisco( ARCHIVO_ISLA5 );

    return 0;
}
