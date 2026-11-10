#include "FileSystem.h"
#include <cstring>
#include <ctime>
#include <iostream>

Filesystem::Filesystem(const std::string& ruta) : ruta_(ruta), bitmap_(archivo_) {
    abrir();
}

Filesystem::~Filesystem() {
    if (archivo_.is_open()) archivo_.close();
}

void Filesystem::abrir() {
    // si ya estaba abierto, open() fallaria y dejaria el failbit encendido
    if (archivo_.is_open()) archivo_.close();
    archivo_.clear();
    archivo_.open(ruta_, std::ios::in | std::ios::out | std::ios::binary);
}

bool Filesystem::archivoValido() {
    if (!archivo_.is_open()) return false;

    Superbloque sb{};
    archivo_.clear();
    leerSuperbloque(sb);
    if (!archivo_) {
        archivo_.clear();
        return false;
    }
    return std::memcmp(sb.firma, FIRMA_CAFETERIA, sizeof(sb.firma)) == 0;
}

std::mutex& Filesystem::mutex() {
    return mutex_;
}

void Filesystem::leerSuperbloque(Superbloque& sb) {
    leerBloque(BLOQUE_SUPERBLOQUE, &sb);
}

void Filesystem::escribirSuperbloque(const Superbloque& sb) {
    escribirBloque(BLOQUE_SUPERBLOQUE, &sb);
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
    // superbloque, bitmap e i-nodo del directorio nunca se liberan
    if (numBloque < BLOQUES_RESERVADOS || numBloque >= NUM_BLOQUES_CAFETERIA) return;

    // se limpia el contenido para que el bloque no conserve datos viejos
    char bloqueVacio[TAM_BLOQUE] = {0};
    escribirBloque(numBloque, bloqueVacio);
    bitmap_.liberarBloque(numBloque);
}

int Filesystem::bloquesLibres() {
    return bitmap_.bloquesLibres();
}

void Filesystem::crearArchivo(const std::string& nombreNegocio) {
    // se cierra antes de que el ofstream trunque el mismo archivo
    if (archivo_.is_open()) archivo_.close();
    {
        std::ofstream nuevo(ruta_, std::ios::out | std::ios::binary);
        char bloqueVacio[TAM_BLOQUE] = {0};
        for (int b = 0; b < NUM_BLOQUES_CAFETERIA; b++) {
            nuevo.write(bloqueVacio, TAM_BLOQUE);
        }
    }
    abrir();

    // bloque 0: superbloque
    Superbloque sb{};
    std::memcpy(sb.firma, FIRMA_CAFETERIA, sizeof(sb.firma));
    std::strncpy(sb.nombreNegocio, nombreNegocio.c_str(), sizeof(sb.nombreNegocio) - 1);
    sb.tamBloque = TAM_BLOQUE;
    sb.numBloques = NUM_BLOQUES_CAFETERIA;
    sb.numBodegas = 0;
    sb.inodoDirectorio = BLOQUE_INODO_DIRECTORIO;
    escribirSuperbloque(sb);

    // bloque 1: bitmap con 0, 1 y 2 ocupados
    bitmap_.inicializar(BLOQUES_RESERVADOS);

    // bloque 2: i-nodo del directorio, todavia sin bloques de datos
    Inodo dir{};
    dir.tipo = INODO_DIRECTORIO;
    dir.fechaModificacion = (uint32_t)time(nullptr);
    for (int i = 0; i < PUNTEROS_INODO; i++) dir.bloques[i] = BLOQUE_NULO;
    escribirInodo(BLOQUE_INODO_DIRECTORIO, dir);
}

// ---------- i-nodos ----------

void Filesystem::leerInodo(uint16_t numInodo, Inodo& inodo) {
    leerBloque(numInodo, &inodo);
}

void Filesystem::escribirInodo(uint16_t numInodo, const Inodo& inodo) {
    escribirBloque(numInodo, &inodo);
}

uint16_t Filesystem::crearInodo(uint8_t tipo) {
    int bloque = pedirBloque();
    if (bloque == -1) return BLOQUE_NULO;

    Inodo inodo{};
    inodo.tipo = tipo;
    inodo.fechaModificacion = (uint32_t)time(nullptr);
    for (int i = 0; i < PUNTEROS_INODO; i++) inodo.bloques[i] = BLOQUE_NULO;
    escribirInodo(bloque, inodo);
    return bloque;
}

void Filesystem::liberarInodo(uint16_t numInodo) {
    Inodo inodo;
    leerInodo(numInodo, inodo);
    for (int i = 0; i < inodo.cantidadBloques; i++) {
        liberarBloque(inodo.bloques[i]);
    }
    liberarBloque(numInodo);
}

std::string Filesystem::leerArchivo(uint16_t numInodo) {
    Inodo inodo;
    leerInodo(numInodo, inodo);

    std::string contenido;
    contenido.reserve(inodo.tamano);
    char bloque[TAM_BLOQUE];

    // se recorren los punteros en orden; el ultimo bloque puede estar lleno solo en parte
    for (int i = 0; i < inodo.cantidadBloques; i++) {
        leerBloque(inodo.bloques[i], bloque);
        size_t faltan = inodo.tamano - contenido.size();
        contenido.append(bloque, faltan < (size_t)TAM_BLOQUE ? faltan : TAM_BLOQUE);
    }
    return contenido;
}

bool Filesystem::escribirArchivo(uint16_t numInodo, const std::string& contenido) {
    if (contenido.size() > (size_t)MAX_TAMANO_ARCHIVO) return false;

    Inodo inodo;
    leerInodo(numInodo, inodo);

    int necesarios = (contenido.size() + TAM_BLOQUE - 1) / TAM_BLOQUE;

    // se revisa antes de tocar nada, para no dejar el archivo escrito a medias
    if (necesarios > inodo.cantidadBloques &&
        necesarios - inodo.cantidadBloques > bloquesLibres()) {
        return false;
    }

    // pedir los bloques que falten
    while (inodo.cantidadBloques < necesarios) {
        inodo.bloques[inodo.cantidadBloques] = pedirBloque();
        inodo.cantidadBloques++;
    }

    // devolver al bitmap los que sobren (el archivo se achico)
    while (inodo.cantidadBloques > necesarios) {
        inodo.cantidadBloques--;
        liberarBloque(inodo.bloques[inodo.cantidadBloques]);
        inodo.bloques[inodo.cantidadBloques] = BLOQUE_NULO;
    }

    // copiar el contenido bloque por bloque; el resto del ultimo bloque queda en ceros
    for (int i = 0; i < necesarios; i++) {
        char bloque[TAM_BLOQUE] = {0};
        size_t desde = (size_t)i * TAM_BLOQUE;
        size_t cuantos = contenido.size() - desde;
        if (cuantos > (size_t)TAM_BLOQUE) cuantos = TAM_BLOQUE;
        std::memcpy(bloque, contenido.data() + desde, cuantos);
        escribirBloque(inodo.bloques[i], bloque);
    }

    inodo.tamano = contenido.size();
    inodo.fechaModificacion = (uint32_t)time(nullptr);
    escribirInodo(numInodo, inodo);
    return true;
}

// ---------- directorio de bodegas ----------
// el directorio es un archivo mas: su contenido es un arreglo de EntradaDirectorio

std::vector<EntradaDirectorio> Filesystem::leerDirectorio() {
    std::string contenido = leerArchivo(BLOQUE_INODO_DIRECTORIO);
    std::vector<EntradaDirectorio> entradas(contenido.size() / sizeof(EntradaDirectorio));
    std::memcpy(entradas.data(), contenido.data(), entradas.size() * sizeof(EntradaDirectorio));
    return entradas;
}

bool Filesystem::escribirDirectorio(const std::vector<EntradaDirectorio>& entradas) {
    std::string contenido(reinterpret_cast<const char*>(entradas.data()),
                          entradas.size() * sizeof(EntradaDirectorio));
    return escribirArchivo(BLOQUE_INODO_DIRECTORIO, contenido);
}

static std::string textoCampo(const char* campo, size_t tam) {
    return std::string(campo, strnlen(campo, tam));
}

uint16_t Filesystem::registrarBodega(const std::string& nombre) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EntradaDirectorio> entradas = leerDirectorio();

    // se reutiliza una casilla libre; si no hay, el directorio crece una entrada
    size_t pos = 0;
    while (pos < entradas.size() && entradas[pos].estado == 1) pos++;
    if (pos == entradas.size()) entradas.push_back(EntradaDirectorio{});

    uint16_t inodo = crearInodo(INODO_BODEGA);
    if (inodo == BLOQUE_NULO) {
        std::cerr << "no hay bloques libres para el i-nodo de la bodega\n";
        return BLOQUE_NULO;
    }

    EntradaDirectorio nueva{};
    std::strncpy(nueva.nombre, nombre.c_str(), sizeof(nueva.nombre) - 1);
    nueva.estado = 1;
    nueva.inodo = inodo;
    nueva.fechaCreacion = (uint32_t)time(nullptr);
    entradas[pos] = nueva;

    if (!escribirDirectorio(entradas)) {
        std::cerr << "no hay bloques libres para crecer el directorio de bodegas\n";
        liberarInodo(inodo);
        return BLOQUE_NULO;
    }

    Superbloque sb;
    leerSuperbloque(sb);
    sb.numBodegas++;
    escribirSuperbloque(sb);
    return inodo;
}

bool Filesystem::buscarBodega(const std::string& nombre, EntradaDirectorio& resultado) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& entrada : leerDirectorio()) {
        if (entrada.estado == 1 && nombre == textoCampo(entrada.nombre, sizeof(entrada.nombre))) {
            resultado = entrada;
            return true;
        }
    }
    return false;
}

std::vector<EntradaDirectorio> Filesystem::listarBodegas() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EntradaDirectorio> bodegas;
    for (const auto& entrada : leerDirectorio()) {
        if (entrada.estado == 1) bodegas.push_back(entrada);
    }
    return bodegas;
}

bool Filesystem::eliminarBodega(const std::string& nombre) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<EntradaDirectorio> entradas = leerDirectorio();
    for (auto& entrada : entradas) {
        if (entrada.estado == 1 && nombre == textoCampo(entrada.nombre, sizeof(entrada.nombre))) {
            uint16_t inodo = entrada.inodo;

            // la casilla queda en ceros (estado 0) y registrarBodega la puede reutilizar
            entrada = EntradaDirectorio{};
            escribirDirectorio(entradas);
            liberarInodo(inodo);

            Superbloque sb;
            leerSuperbloque(sb);
            sb.numBodegas--;
            escribirSuperbloque(sb);
            return true;
        }
    }
    return false;
}
