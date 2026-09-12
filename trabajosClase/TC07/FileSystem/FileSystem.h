#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include "Formatos.h"
#include "Bitmap.h"

// maneja cafeteria.dat completo: superbloque, bitmap, y directorio de bodegas.
// Bodega usa esta clase para pedir bloques y leer/escribir dentro del mismo archivo,
// ya que todas las bodegas viven adentro de este unico archivo.
class Filesystem {
public:
    Filesystem(const std::string& ruta);
    ~Filesystem();

    void crearArchivo(const std::string& nombreNegocio);

    // acceso generico a un bloque de 256 bytes
    void leerBloque(int numBloque, void* destino);
    void escribirBloque(int numBloque, const void* origen);

    int pedirBloque();
    void liberarBloque(int numBloque);

    // directorio de bodegas, con crecimiento por punteros
    uint16_t registrarBodega(const std::string& nombre, const std::string& id);
    bool buscarBodega(const std::string& id, DirBodega& resultado);
    std::vector<DirBodega> listarBodegas();

    // Bodega envuelve sus propias operaciones con este mutex, porque todas
    // las bodegas comparten el mismo archivo y el mismo bitmap
    std::mutex& mutex();

private:
    std::string ruta_;
    std::fstream archivo_;
    Bitmap bitmap_;
    std::mutex mutex_;

    void abrir();
    void leerSuperbloque(Superbloque& sb);
    void escribirSuperbloque(const Superbloque& sb);
};

#endif