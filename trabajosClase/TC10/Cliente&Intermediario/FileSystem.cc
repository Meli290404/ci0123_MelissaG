#include "FileSystem.h"
#include <cstring>
#include <iostream>

Filesystem::Filesystem(const std::string& ruta) : ruta_(ruta), bitmap_(archivo_) {
    abrir();
}

Filesystem::~Filesystem() {
    if (archivo_.is_open()) archivo_.close();
}

void Filesystem::abrir() {
    archivo_.open(ruta_, std::ios::in | std::ios::out | std::ios::binary);
}

std::mutex& Filesystem::mutex() {
    return mutex_;
}

void Filesystem::leerSuperbloque(Superbloque& sb) {
    archivo_.seekg(0);
    archivo_.read(reinterpret_cast<char*>(&sb), sizeof(Superbloque));
}

void Filesystem::escribirSuperbloque(const Superbloque& sb) {
    archivo_.seekp(0);
    archivo_.write(reinterpret_cast<const char*>(&sb), sizeof(Superbloque));
    archivo_.flush();
}

void Filesystem::leerBloque(int numBloque, void* destino) {
    archivo_.seekg(numBloque * TAM_BLOQUE);
    archivo_.read(reinterpret_cast<char*>(destino), TAM_BLOQUE);
}

void Filesystem::escribirBloque(int numBloque, const void* origen) {
    archivo_.seekp(numBloque * TAM_BLOQUE);
    archivo_.write(reinterpret_cast<const char*>(origen), TAM_BLOQUE);
    archivo_.flush();
}

int Filesystem::pedirBloque() {
    return bitmap_.pedirBloque();
}

void Filesystem::liberarBloque(int numBloque) {
    bitmap_.liberarBloque(numBloque);
}

int Filesystem::bloquesLibres() {
    return bitmap_.bloquesLibres();
}

void Filesystem::crearArchivo(const std::string& nombreNegocio) {
    std::ofstream nuevo(ruta_, std::ios::out | std::ios::binary);

    // bloque 0: superbloque. el directorio de bodegas arranca en el bloque 2
    Superbloque sb{};
    std::memcpy(sb.firma, FIRMA_CAFETERIA, sizeof(sb.firma));
    std::strncpy(sb.nombreNegocio, nombreNegocio.c_str(), sizeof(sb.nombreNegocio) - 1);
    sb.numBodegas = 0;
    sb.bloqueDirBodegas = 2;
    sb.bloquesLibres = NUM_BLOQUES_CAFETERIA - 3;
    nuevo.write(reinterpret_cast<char*>(&sb), TAM_BLOQUE);

    // bloque 1: bitmap, con 0, 1 y 2 marcados como ocupados desde ya
    uint8_t mapa[BITMAP_BYTES] = {0};
    mapa[0] = 0b11100000;
    nuevo.write(reinterpret_cast<char*>(mapa), BITMAP_BYTES);
    char rellenoBitmap[TAM_BLOQUE - BITMAP_BYTES] = {0};
    nuevo.write(rellenoBitmap, sizeof(rellenoBitmap));

    // bloque 2: primer bloque del directorio de bodegas, vacio
    BloqueDirBodegas dirBodegas{};
    dirBodegas.encabezado.siguienteBloque = BLOQUE_NULO;
    dirBodegas.encabezado.casillasUsadas = 0;
    nuevo.write(reinterpret_cast<char*>(&dirBodegas), TAM_BLOQUE);

    // el resto queda en ceros, se llena conforme se crean categorias e indices
    char bloqueVacio[TAM_BLOQUE] = {0};
    for (int b = 3; b < NUM_BLOQUES_CAFETERIA; b++) {
        nuevo.write(bloqueVacio, TAM_BLOQUE);
    }

    nuevo.close();
    abrir();
}

uint16_t Filesystem::registrarBodega(const std::string& nombre, const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);

    Superbloque sb;
    leerSuperbloque(sb);

    int bloqueActual = sb.bloqueDirBodegas;
    BloqueDirBodegas dir{};

    while (true) {
        leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 0) {
                int bloqueDirCategorias = pedirBloque();
                if (bloqueDirCategorias == -1) {
                    std::cerr << "no hay bloques libres para registrar la bodega\n";
                    return BLOQUE_NULO;
                }

                DirBodega nueva{};
                std::strncpy(nueva.nombre, nombre.c_str(), sizeof(nueva.nombre) - 1);
                std::strncpy(nueva.id, id.c_str(), sizeof(nueva.id) - 1);
                nueva.estado = 1;
                nueva.bloqueDirCategorias = bloqueDirCategorias;
                dir.casillas[i] = nueva;
                dir.encabezado.casillasUsadas++;
                escribirBloque(bloqueActual, &dir);

                BloqueDirCategorias dirCat{};
                dirCat.encabezado.siguienteBloque = BLOQUE_NULO;
                dirCat.encabezado.casillasUsadas = 0;
                escribirBloque(bloqueDirCategorias, &dirCat);

                sb.numBodegas++;
                escribirSuperbloque(sb);
                return bloqueDirCategorias;
            }
        }

        // este bloque esta lleno: seguir la cadena, o crear el siguiente bloque
        if (dir.encabezado.siguienteBloque != BLOQUE_NULO) {
            bloqueActual = dir.encabezado.siguienteBloque;
            continue;
        }

        int nuevoBloque = pedirBloque();
        if (nuevoBloque == -1) {
            std::cerr << "no hay bloques libres para crecer el directorio de bodegas\n";
            return BLOQUE_NULO;
        }
        dir.encabezado.siguienteBloque = nuevoBloque;
        escribirBloque(bloqueActual, &dir);

        BloqueDirBodegas nuevoDir{};
        nuevoDir.encabezado.siguienteBloque = BLOQUE_NULO;
        nuevoDir.encabezado.casillasUsadas = 0;
        escribirBloque(nuevoBloque, &nuevoDir);

        bloqueActual = nuevoBloque; // el while vuelve a empezar y ya encuentra espacio
    }
}

bool Filesystem::buscarBodega(const std::string& id, DirBodega& resultado) {
    std::lock_guard<std::mutex> lock(mutex_);

    Superbloque sb;
    leerSuperbloque(sb);

    int bloqueActual = sb.bloqueDirBodegas;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirBodegas dir;
        leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1 &&
                id == std::string(dir.casillas[i].id, strnlen(dir.casillas[i].id, sizeof(dir.casillas[i].id)))) {
                resultado = dir.casillas[i];
                return true;
            }
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return false;
}

std::vector<DirBodega> Filesystem::listarBodegas() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<DirBodega> bodegas;
    Superbloque sb;
    leerSuperbloque(sb);

    int bloqueActual = sb.bloqueDirBodegas;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirBodegas dir;
        leerBloque(bloqueActual, &dir);
        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1) bodegas.push_back(dir.casillas[i]);
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return bodegas;
}