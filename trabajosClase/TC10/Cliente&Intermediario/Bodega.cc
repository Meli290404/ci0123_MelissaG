#include "Bodega.h"
#include <cstring>
#include <iostream>

Bodega::Bodega(Filesystem& fs, uint16_t bloqueDirCategorias)
    : fs_(fs), bloqueDirCategorias_(bloqueDirCategorias) {}

bool Bodega::ubicarCasillaCategoria(const std::string& id, int& bloqueOut, int& posOut) {
    int bloqueActual = bloqueDirCategorias_;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirCategorias dir;
        fs_.leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1 &&
                id == std::string(dir.casillas[i].id, strnlen(dir.casillas[i].id, sizeof(dir.casillas[i].id)))) {
                bloqueOut = bloqueActual;
                posOut = i;
                return true;
            }
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return false;
}

void Bodega::crearCategoria(const std::string& nombre, const std::string& id) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloqueActual = bloqueDirCategorias_;
    BloqueDirCategorias dir{};

    while (true) {
        fs_.leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 0) {
                int bloqueIndice = fs_.pedirBloque();
                if (bloqueIndice == -1) {
                    std::cerr << "no hay bloques libres para crear la categoria\n";
                    return;
                }

                DirCategoria nueva{};
                std::strncpy(nueva.nombre, nombre.c_str(), sizeof(nueva.nombre) - 1);
                std::strncpy(nueva.id, id.c_str(), sizeof(nueva.id) - 1);
                nueva.estado = 1;
                nueva.bloqueIndice = bloqueIndice;
                dir.casillas[i] = nueva;
                dir.encabezado.casillasUsadas++;
                fs_.escribirBloque(bloqueActual, &dir);

                BloqueIndice indiceVacio{};
                indiceVacio.cantidadBloques = 0;
                indiceVacio.cantidadProductos = 0;
                fs_.escribirBloque(bloqueIndice, &indiceVacio);
                return;
            }
        }

        if (dir.encabezado.siguienteBloque != BLOQUE_NULO) {
            bloqueActual = dir.encabezado.siguienteBloque;
            continue;
        }

        int nuevoBloque = fs_.pedirBloque();
        if (nuevoBloque == -1) {
            std::cerr << "no hay bloques libres para crecer el directorio de categorias\n";
            return;
        }
        dir.encabezado.siguienteBloque = nuevoBloque;
        fs_.escribirBloque(bloqueActual, &dir);

        BloqueDirCategorias nuevoDir{};
        nuevoDir.encabezado.siguienteBloque = BLOQUE_NULO;
        nuevoDir.encabezado.casillasUsadas = 0;
        fs_.escribirBloque(nuevoBloque, &nuevoDir);

        bloqueActual = nuevoBloque;
    }
}

bool Bodega::buscarCategoria(const std::string& id, DirCategoria& resultado) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloque, pos;
    if (!ubicarCasillaCategoria(id, bloque, pos)) return false;

    BloqueDirCategorias dir;
    fs_.leerBloque(bloque, &dir);
    resultado = dir.casillas[pos];
    return true;
}

bool Bodega::agregarProducto(const std::string& idCategoria, const std::string& nombreProducto, int cantidad, uint32_t precioCentavos) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloqueCasilla, posCasilla;
    if (!ubicarCasillaCategoria(idCategoria, bloqueCasilla, posCasilla)) {
        std::cerr << "categoria no encontrada: " << idCategoria << "\n";
        return false;
    }

    BloqueDirCategorias dirCat;
    fs_.leerBloque(bloqueCasilla, &dirCat);
    DirCategoria categoria = dirCat.casillas[posCasilla];

    BloqueIndice indice;
    fs_.leerBloque(categoria.bloqueIndice, &indice);

    int posicionLibre = -1;
    int bloqueDestino = -1;

    // buscar espacio en los bloques de datos que ya tiene la categoria
    for (int b = 0; b < indice.cantidadBloques && bloqueDestino == -1; b++) {
        RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
        fs_.leerBloque(indice.bloquesDatos[b], bloqueDatos);
        for (int p = 0; p < PRODUCTOS_POR_BLOQUE; p++) {
            if (bloqueDatos[p].estado == 0) {
                bloqueDestino = indice.bloquesDatos[b];
                posicionLibre = p;
                break;
            }
        }
    }

    // si no hay espacio, pedir un bloque nuevo y agregarlo al indice
    if (bloqueDestino == -1) {
        int nuevoBloque = fs_.pedirBloque();
        if (nuevoBloque == -1) {
            std::cerr << "no hay bloques libres para mas productos\n";
            return false;
        }
        indice.bloquesDatos[indice.cantidadBloques] = nuevoBloque;
        indice.cantidadBloques++;
        bloqueDestino = nuevoBloque;
        posicionLibre = 0;

        RegistroProducto bloqueNuevo[PRODUCTOS_POR_BLOQUE] = {};
        fs_.escribirBloque(bloqueDestino, bloqueNuevo);
    }

    RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
    fs_.leerBloque(bloqueDestino, bloqueDatos);

    RegistroProducto nuevo{};
    std::strncpy(nuevo.nombre, nombreProducto.c_str(), sizeof(nuevo.nombre) - 1);
    nuevo.cantidad = cantidad;
    nuevo.precioCentavos = precioCentavos;
    nuevo.estado = 1;
    bloqueDatos[posicionLibre] = nuevo;
    fs_.escribirBloque(bloqueDestino, bloqueDatos);

    indice.cantidadProductos++;
    fs_.escribirBloque(categoria.bloqueIndice, &indice);
    return true;
}

std::vector<DirCategoria> Bodega::listarCategorias() {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<DirCategoria> categorias;
    int bloqueActual = bloqueDirCategorias_;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirCategorias dir;
        fs_.leerBloque(bloqueActual, &dir);
        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1) categorias.push_back(dir.casillas[i]);
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return categorias;
}

std::vector<RegistroProducto> Bodega::listarProductos(const std::string& idCategoria) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<RegistroProducto> productos;
    int bloqueCasilla, posCasilla;
    if (!ubicarCasillaCategoria(idCategoria, bloqueCasilla, posCasilla)) return productos;

    BloqueDirCategorias dirCat;
    fs_.leerBloque(bloqueCasilla, &dirCat);
    DirCategoria categoria = dirCat.casillas[posCasilla];

    BloqueIndice indice;
    fs_.leerBloque(categoria.bloqueIndice, &indice);

    for (int b = 0; b < indice.cantidadBloques; b++) {
        RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
        fs_.leerBloque(indice.bloquesDatos[b], bloqueDatos);
        for (int p = 0; p < PRODUCTOS_POR_BLOQUE; p++) {
            if (bloqueDatos[p].estado == 1) productos.push_back(bloqueDatos[p]);
        }
    }
    return productos;
}