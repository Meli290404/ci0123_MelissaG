#include <iostream>
#include <string>
#include <cctype>
#include <cstdint>
#include <vector>
#include <cstring>
#include <fstream>

#include "FileSystem.h"
#include "AdministradorBodegas.h"

static void mostrarUso(const char* programa) {
    std::cout << "uso:\n"
              << "  " << programa << " <archivo> crear <nombreNegocio>\n"
              << "  " << programa << " <archivo> listar\n"
              << "  " << programa << " <archivo> libres\n"
              << "  " << programa << " <archivo> agregar <bodega>\n"
              << "  " << programa << " <archivo> eliminar <bodega>\n"
              << "  " << programa << " <archivo> producto <bodega> <categoria> <nombre> <cantidad> <precioCentavos>\n"
              << "  " << programa << " <archivo> descontar <producto> <cantidad>\n"
              << "  " << programa << " <archivo> inodos\n"
              << "  " << programa << " <archivo> ver <bodega>\n"
              << "  " << programa << " <archivo> extraer <categoria> [archivoSalida]\n";
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
        std::cout << bodega.nombre << "\n";
        for (auto& categoria : bodega.categorias) {
            std::cout << "    " << categoria.id << "\n";
            for (auto& p : categoria.productos) {
                std::cout << "        " << p.nombre << " | precio " << p.precio << " | stock " << p.stock << "\n";
            }
        }
    }
}

static std::string textoCampo(const char* campo, size_t tam) {
    return std::string(campo, strnlen(campo, tam));
}

// muestra el superbloque, el i-nodo del directorio y el i-nodo de cada bodega con sus bloques
static void mostrarInodos(Filesystem& fs) {
    Superbloque sb;
    fs.leerSuperbloque(sb);
    std::cout << "superbloque: " << textoCampo(sb.nombreNegocio, sizeof(sb.nombreNegocio))
              << " | bloques de " << sb.tamBloque << " bytes: " << sb.numBloques
              << " | bodegas: " << sb.numBodegas << " | libres: " << fs.bloquesLibres() << "\n";

    auto mostrar = [&](const std::string& titulo, uint16_t numInodo) {
        Inodo inodo;
        fs.leerInodo(numInodo, inodo);
        std::cout << titulo << " -> i-nodo en bloque " << numInodo << ", " << inodo.tamano
                  << " bytes, " << inodo.cantidadBloques << " bloques:";
        for (int i = 0; i < inodo.cantidadBloques; i++) std::cout << " " << inodo.bloques[i];
        std::cout << "\n";
    };

    mostrar("directorio", sb.inodoDirectorio);
    for (auto& entrada : fs.listarBodegas()) {
        mostrar(textoCampo(entrada.nombre, sizeof(entrada.nombre)), entrada.inodo);
    }
}

// copia afuera del sistema de archivos las lineas de una categoria, en el mismo formato en que
// estan guardadas. si no se da archivoSalida se usa <categoria>.csv
static bool extraer(Filesystem& fs, const std::string& categoria, std::string salida, std::string& error) {
    if (salida.empty()) salida = categoria + ".csv";

    std::vector<LineaProducto> lineas;
    std::vector<std::string> existentes;
    for (auto& entrada : fs.listarBodegas()) {
        Bodega bodega(fs, entrada.inodo, textoCampo(entrada.nombre, sizeof(entrada.nombre)));
        for (auto& c : bodega.listarCategorias()) existentes.push_back(c);
        if (lineas.empty() && bodega.tieneCategoria(categoria)) lineas = bodega.listarProductos(categoria);
    }

    if (lineas.empty()) {
        error = "no existe la categoria " + categoria + "; las categorias del archivo son:";
        for (auto& c : existentes) error += " " + c;
        return false;
    }

    std::ofstream archivo(salida);
    if (!archivo) {
        error = "no se pudo crear " + salida;
        return false;
    }
    for (auto& linea : lineas) {
        std::string texto = lineaATexto(linea);
        if (!texto.empty() && texto.back() == '\n') texto.pop_back();
        std::cout << texto << "\n";
        archivo << texto << "\n";
    }
    std::cout << "categoria " << categoria << " de " << lineas[0].bodega << ": " << lineas.size()
              << " productos extraidos a " << salida << "\n";
    return true;
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
    } else if (comando == "agregar" && argc == 4) {
        ok = admin.agregarBodega(argv[3], error);
    } else if (comando == "eliminar" && argc == 4) {
        ok = admin.eliminarBodega(argv[3], error);
    } else if (comando == "producto" && argc == 8) {
        if (!esNumero(argv[6]) || !esNumero(argv[7])) {
            ok = false;
            error = "cantidad y precioCentavos deben ser numeros enteros";
        } else {
            ok = admin.agregarProducto(argv[3], argv[4], argv[5], std::stoi(argv[6]),
                                       (uint32_t)std::stoul(argv[7]), error);
        }
    } else if (comando == "inodos" && argc == 3) {
        mostrarInodos(fs);
    } else if (comando == "ver" && argc == 4) {
        // imprime el archivo de texto de la bodega tal como esta guardado
        EntradaDirectorio entrada;
        if (!fs.buscarBodega(argv[3], entrada)) {
            ok = false;
            error = std::string("no existe una bodega llamada ") + argv[3];
        } else {
            std::lock_guard<std::mutex> lock(fs.mutex());
            std::cout << fs.leerArchivo(entrada.inodo);
        }
    } else if (comando == "extraer" && (argc == 4 || argc == 5)) {
        ok = extraer(fs, argv[3], argc == 5 ? argv[4] : "", error);
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
    if (comando != "listar" && comando != "libres" && comando != "inodos" && comando != "ver" &&
        comando != "extraer") std::cout << "listo\n";
    return 0;
}