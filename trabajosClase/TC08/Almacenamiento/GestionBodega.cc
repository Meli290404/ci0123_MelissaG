#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>
#include <cstring>
#include <cstdio>

#include "FileSystem.h"
#include "Bodega.h"

// nombre del archivo que actua como disco simulado, compartido por las dos bodegas
static const char* nombreArchivo = "cafeteria.dat";

// descarta lo que haya quedado en el buffer de entrada despues de un cin invalido,
// para que la siguiente lectura no arrastre esos caracteres
void limpiarEntrada() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// pide un entero por consola y repite la pregunta mientras la entrada no sea valida
int leerEntero(const std::string& prompt) {
    int valor;
    std::cout << prompt;
    while (!(std::cin >> valor)) {
        std::cout << "entrada invalida, intente de nuevo: ";
        limpiarEntrada();
    }
    limpiarEntrada();
    return valor;
}

// pide una linea de texto completa por consola
std::string leerLinea(const std::string& prompt) {
    std::string valor;
    std::cout << prompt;
    std::getline(std::cin, valor);
    return valor;
}

// revisa si el archivo ya existe en disco, sin crearlo ni modificarlo
bool existeArchivo(const std::string& ruta) {
    std::ifstream f(ruta);
    return f.good();
}

// crea cafeteria.dat desde cero con las dos bodegas y su catalogo inicial,
// igual a como lo hace Intermediario.cc al arrancar por primera vez
void crearArchivoConDatosDeEjemplo(Filesystem& fs) {
    std::cout << nombreArchivo << " no existe todavia. Creando uno nuevo con Bodega-1, "
                 "Bodega-2 y su catalogo inicial...\n";

    fs.crearArchivo("TicAmazon");

    uint16_t bloqueBod1 = fs.registrarBodega("Bodega-1", "bod1");
    uint16_t bloqueBod2 = fs.registrarBodega("Bodega-2", "bod2");

    Bodega bodega1(fs, bloqueBod1);
    bodega1.crearCategoria("Bebidas frias", "bebfr");
    bodega1.crearCategoria("Bebidas calientes", "bebca");
    bodega1.crearCategoria("Postres", "postr");
    bodega1.agregarProducto("bebfr", "CafeFrio", 20, 150000);
    bodega1.agregarProducto("bebfr", "Limonada", 15, 100000);
    bodega1.agregarProducto("bebca", "CafeCaliente", 30, 140000);
    bodega1.agregarProducto("postr", "Tiramisu", 10, 250000);

    Bodega bodega2(fs, bloqueBod2);
    bodega2.crearCategoria("Caramelos", "caram");
    bodega2.crearCategoria("Reposteria", "repos");
    bodega2.agregarProducto("caram", "Chupeta", 50, 30000);
    bodega2.agregarProducto("repos", "PanDulce", 25, 80000);

    std::cout << "Listo: " << nombreArchivo << " creado con datos de ejemplo.\n";
}

// imprime el id y el nombre de cada bodega registrada en el directorio del archivo principal
void mostrarBodegas(const std::vector<DirBodega>& bodegasInfo) {
    std::cout << "\n-- Bodegas registradas --\n";
    for (auto& b : bodegasInfo) {
        std::string nombre(b.nombre, strnlen(b.nombre, sizeof(b.nombre)));
        std::string id(b.id, strnlen(b.id, sizeof(b.id)));
        std::cout << "  " << id << " (" << nombre << ")\n";
    }
}

// busca en bodegasInfo el id pedido y devuelve (creando si hace falta) la instancia
// de Bodega correspondiente, reutilizando las que ya se abrieron antes en esta corrida
Bodega* obtenerBodega(Filesystem& fs, const std::string& idBodega,
                       const std::vector<DirBodega>& bodegasInfo, std::vector<Bodega*>& instancias) {
    for (size_t i = 0; i < bodegasInfo.size(); i++) {
        std::string id(bodegasInfo[i].id, strnlen(bodegasInfo[i].id, sizeof(bodegasInfo[i].id)));
        if (id == idBodega) {
            if (instancias[i] == nullptr) {
                instancias[i] = new Bodega(fs, bodegasInfo[i].bloqueDirCategorias);
            }
            return instancias[i];
        }
    }
    return nullptr;
}

// imprime el id y el nombre de cada categoria que tiene esa bodega
void mostrarCategorias(Bodega* bodega) {
    std::cout << "\n-- Categorias --\n";
    for (auto& c : bodega->listarCategorias()) {
        std::string nombre(c.nombre, strnlen(c.nombre, sizeof(c.nombre)));
        std::string id(c.id, strnlen(c.id, sizeof(c.id)));
        std::cout << "  " << id << " (" << nombre << ")\n";
    }
}

// imprime nombre, stock y precio de cada producto de una categoria
void mostrarProductos(Bodega* bodega, const std::string& idCategoria) {
    auto productos = bodega->listarProductos(idCategoria);
    if (productos.empty()) {
        std::cout << "  (sin productos en '" << idCategoria << "', o la categoria no existe)\n";
        return;
    }
    std::cout << "\n-- Productos en " << idCategoria << " --\n";
    for (auto& p : productos) {
        std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
        char precioStr[16];
        snprintf(precioStr, sizeof(precioStr), "%.2f", p.precioCentavos / 100.0);
        std::cout << "  " << nombre << " | stock: " << p.cantidad << " | precio: " << precioStr << "\n";
    }
}

int main() {
    // si el archivo no existe, se crea antes de abrirlo con datos de ejemplo,
    // asi el programa no depende de haber corrido intermediario antes
    bool primeraVez = !existeArchivo(nombreArchivo);

    Filesystem fs(nombreArchivo);

    if (primeraVez) {
        crearArchivoConDatosDeEjemplo(fs);
    }

    std::vector<DirBodega> bodegasInfo = fs.listarBodegas();
    if (bodegasInfo.empty()) {
        std::cerr << "No se encontraron bodegas en " << nombreArchivo << ". El archivo puede estar corrupto.\n";
        return 1;
    }
    // una instancia de Bodega por cada bodega registrada, se crean solo cuando se usan por primera vez
    std::vector<Bodega*> instancias(bodegasInfo.size(), nullptr);

    std::cout << "--- Gestion de Bodegas TicAmazon ---\n";
    std::cout << "Archivo abierto: " << nombreArchivo << "\n";

    for (;;) {
        std::cout << "\n1) Listar bodegas\n"
                     "2) Listar categorias de una bodega\n"
                     "3) Listar productos de una categoria\n"
                     "4) Insertar / agregar stock a un producto\n"
                     "5) Extraer / quitar stock de un producto\n"
                     "6) Salir\n";
        int opcion = leerEntero("Opcion: ");

        if (opcion == 6) break;
        if (opcion == 1) { mostrarBodegas(bodegasInfo); continue; }
        if (opcion < 2 || opcion > 5) { std::cout << "Opcion invalida\n"; continue; }

        std::string idBodega = leerLinea("Id de bodega (ej. bod1): ");
        Bodega* bodega = obtenerBodega(fs, idBodega, bodegasInfo, instancias);
        if (bodega == nullptr) { std::cout << "Bodega no encontrada\n"; continue; }

        if (opcion == 2) { mostrarCategorias(bodega); continue; }

        std::string idCategoria = leerLinea("Id de categoria (ej. bebfr): ");
        if (opcion == 3) { mostrarProductos(bodega, idCategoria); continue; }

        std::string nombreProducto = leerLinea("Nombre del producto: ");

        if (opcion == 4) {
            // insertarStock suma al stock existente si el producto ya esta en la categoria,
            // o crea un registro nuevo si no existe todavia
            int cantidad = leerEntero("Cantidad a insertar/agregar: ");
            double precio;
            std::cout << "Precio unitario (ej. 1500.00): ";
            std::cin >> precio;
            limpiarEntrada();

            bool ok = bodega->insertarStock(idCategoria, nombreProducto, cantidad,
                                             (uint32_t)(precio * 100 + 0.5));
            std::cout << (ok ? "Stock insertado correctamente.\n" : "No se pudo insertar (revise la categoria).\n");
        } else if (opcion == 5) {
            // extraerStock revisa que haya suficiente cantidad antes de restar
            int cantidad = leerEntero("Cantidad a extraer: ");
            bool ok = bodega->extraerStock(idCategoria, nombreProducto, cantidad);
            std::cout << (ok ? "Stock extraido correctamente.\n" : "No se pudo extraer (revise producto y stock disponible).\n");
        }
    }

    // libera las instancias de Bodega que se llegaron a crear durante la corrida
    for (auto* b : instancias) delete b;
    std::cout << "\n--- Fin ---\n";
    return 0;
}