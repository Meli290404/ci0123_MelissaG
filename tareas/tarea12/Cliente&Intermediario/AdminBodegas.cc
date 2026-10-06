#include <iostream>
#include <string>
#include <cctype>
#include <cstdint>

#include "FileSystem.h"
#include "AdministradorBodegas.h"

static void mostrarUso(const char* programa) {
    std::cout << "uso:\n"
              << "  " << programa << " <archivo> crear <nombreNegocio>\n"
              << "  " << programa << " <archivo> listar\n"
              << "  " << programa << " <archivo> libres\n"
              << "  " << programa << " <archivo> agregar <idBodega> <nombre>\n"
              << "  " << programa << " <archivo> eliminar <idBodega>\n"
              << "  " << programa << " <archivo> categoria <idBodega> <idCategoria> <nombre>\n"
              << "  " << programa << " <archivo> producto <idBodega> <idCategoria> <nombre> <cantidad> <precioCentavos>\n"
              << "  " << programa << " <archivo> descontar <producto> <cantidad>\n";
}

static bool esNumero(const std::string& texto) {
    if (texto.empty() || texto.size() > 9) return false;
    for (char c : texto) {
        if (!isdigit((unsigned char)c)) return false;
    }
    return true;
}

static void listar(AdministradorBodegas& admin) {
    std::vector<BodegaVista> inventario = admin.leerInventario();
    if (inventario.empty()) {
        std::cout << "no hay bodegas registradas\n";
        return;
    }
    for (auto& bodega : inventario) {
        std::cout << bodega.id << "  " << bodega.nombre << "\n";
        for (auto& categoria : bodega.categorias) {
            std::cout << "    " << categoria.id << "  " << categoria.nombre << "\n";
            for (auto& p : categoria.productos) {
                std::cout << "        " << p.nombre << " | precio " << p.precio << " | stock " << p.stock << "\n";
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        mostrarUso(argv[0]);
        return 1;
    }
    std::string ruta = argv[1];
    std::string comando = argv[2];

    Filesystem fs(ruta);

    if (comando == "crear") {
        if (argc != 4) {
            mostrarUso(argv[0]);
            return 1;
        }
        // no se sobrescribe un archivo valido para no perder bodegas por accidente
        if (fs.archivoValido()) {
            std::cerr << ruta << " ya existe; borrelo primero si quiere crearlo de nuevo\n";
            return 1;
        }
        fs.crearArchivo(argv[3]);
        std::cout << "archivo creado: " << ruta << "\n";
        return 0;
    }

    if (!fs.archivoValido()) {
        std::cerr << ruta << " no existe o no es valido, use el comando crear\n";
        return 1;
    }

    AdministradorBodegas admin(fs);
    std::string error;
    bool ok = true;

    if (comando == "listar" && argc == 3) {
        listar(admin);
    } else if (comando == "libres" && argc == 3) {
        std::cout << "bloques libres: " << fs.bloquesLibres() << " de " << NUM_BLOQUES_CAFETERIA << "\n";
    } else if (comando == "agregar" && argc == 5) {
        ok = admin.agregarBodega(argv[3], argv[4], error);
    } else if (comando == "eliminar" && argc == 4) {
        ok = admin.eliminarBodega(argv[3], error);
    } else if (comando == "categoria" && argc == 6) {
        ok = admin.agregarCategoria(argv[3], argv[4], argv[5], error);
    } else if (comando == "producto" && argc == 8) {
        if (!esNumero(argv[6]) || !esNumero(argv[7])) {
            ok = false;
            error = "cantidad y precioCentavos deben ser numeros enteros";
        } else {
            ok = admin.agregarProducto(argv[3], argv[4], argv[5], std::stoi(argv[6]),
                                       (uint32_t)std::stoul(argv[7]), error);
        }
    } else if (comando == "descontar" && argc == 5) {
        if (!esNumero(argv[4])) {
            ok = false;
            error = "la cantidad debe ser un numero entero";
        } else {
            int stockActual = 0;
            ok = admin.descontarStock(argv[3], std::stoi(argv[4]), stockActual, error);
            if (ok) std::cout << "quedan " << stockActual << "\n";
        }
    } else {
        mostrarUso(argv[0]);
        return 1;
    }

    if (!ok) {
        std::cerr << "error: " << error << "\n";
        return 1;
    }
    if (comando != "listar" && comando != "libres") std::cout << "listo\n";
    return 0;
}