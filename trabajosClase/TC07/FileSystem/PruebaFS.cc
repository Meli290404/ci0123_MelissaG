#include <iostream>
#include <thread>
#include <vector>
#include "FileSystem.h"
#include "Bodega.h"

void probarCreacionBasica(Filesystem& fs) {
    uint16_t bloqueBod1 = fs.registrarBodega("Bodega-1", "bod1");
    uint16_t bloqueBod2 = fs.registrarBodega("Bodega-2", "bod2");

    Bodega bodega1(fs, bloqueBod1);
    bodega1.crearCategoria("Bebidas frias", "bebfr");
    bodega1.crearCategoria("Bebidas calientes", "bebca");
    bodega1.crearCategoria("Postres", "postr");

    Bodega bodega2(fs, bloqueBod2);
    bodega2.crearCategoria("Caramelos", "caram");
    bodega2.crearCategoria("Reposteria", "repos");

    bodega1.agregarProducto("bebfr", "Cafe frio", 20, 1000);
    bodega1.agregarProducto("bebfr", "Limonada", 15, 2000);

    std::cout << "-- productos en bebfr --\n";
    for (auto& p : bodega1.listarProductos("bebfr")) {
        std::cout << "  " << p.nombre << " cant=" << p.cantidad
                  << " precio=" << p.precioCentavos << "\n";
    }

    DirBodega db;
    std::cout << "buscarBodega(bod2): " << (fs.buscarBodega("bod2", db) ? "encontrada" : "no encontrada") << "\n";

    DirCategoria dc;
    std::cout << "buscarCategoria(caram): " << (bodega2.buscarCategoria("caram", dc) ? "encontrada" : "no encontrada") << "\n";
}

void probarCrecimientoDeDatos(Filesystem& fs, uint16_t bloqueBod1) {
    // una categoria solo tiene 8 productos por bloque; con 10 productos
    // tiene que pedir un segundo bloque de datos automaticamente
    Bodega bodega1(fs, bloqueBod1);
    for (int i = 0; i < 10; i++) {
        bodega1.agregarProducto("postr", "Postre " + std::to_string(i), 5, 80000 + i);
    }
    auto productos = bodega1.listarProductos("postr");
    std::cout << "-- crecimiento de datos: postr tiene " << productos.size() << " productos (esperado 10) --\n";
}

void probarCrecimientoDeDirectorio(Filesystem& fs, uint16_t bloqueBod2) {
    // el directorio de categorias tiene 7 casillas por bloque; bodega2 ya
    // tiene 2 (caramelos, reposteria), agregamos 6 mas para forzar un segundo bloque
    Bodega bodega2(fs, bloqueBod2);
    for (int i = 0; i < 6; i++) {
        bodega2.crearCategoria("Categoria " + std::to_string(i), "cat" + std::to_string(i));
    }

    DirCategoria dc;
    bool encontrada = bodega2.buscarCategoria("cat5", dc);
    std::cout << "-- crecimiento de directorio: cat5 (deberia estar en el segundo bloque) "
              << (encontrada ? "encontrada" : "NO encontrada") << " --\n";
}

void probarConcurrencia(Filesystem& fs, uint16_t bloqueBod1) {
    // varios hilos agregando productos a la vez a la misma categoria,
    // simulando el modelo de un hilo por solicitud
    Bodega bodega1(fs, bloqueBod1);
    std::vector<std::thread> hilos;

    for (int i = 0; i < 20; i++) {
        hilos.emplace_back([&bodega1, i]() {
            bodega1.agregarProducto("bebca", "Concurrente " + std::to_string(i), 1, 10000);
        });
    }
    for (auto& h : hilos) h.join();

    auto productos = bodega1.listarProductos("bebca");
    std::cout << "-- concurrencia: bebca tiene " << productos.size() << " productos (esperado 20) --\n";
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

    Bodega bodega2(fs, bloqueBod2);
    bodega2.crearCategoria("Caramelos", "caram");
    bodega2.crearCategoria("Reposteria", "repos");

    bodega1.agregarProducto("bebfr", "Cafe frio", 20, 150000);
    bodega1.agregarProducto("bebfr", "Limonada", 15, 100000);

    std::cout << "-- productos en bebfr --\n";
    for (auto& p : bodega1.listarProductos("bebfr")) {
        std::cout << "  " << p.nombre << " cant=" << p.cantidad
                  << " precio=" << p.precioCentavos << "\n";
    }

    probarCrecimientoDeDatos(fs, bloqueBod1);
    probarCrecimientoDeDirectorio(fs, bloqueBod2);
    probarConcurrencia(fs, bloqueBod1);

    std::cout << "bloques libres al final: " << fs.bloquesLibres() << "\n";

    return 0;
}