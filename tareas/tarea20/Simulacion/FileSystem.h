#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include "Formatos.h"
#include "Bitmap.h"

// maneja fs.dat.ci0123 completo: superbloque, bitmap, i-nodos y directorio de bodegas.
// cada bodega es un archivo de texto con su propio i-nodo; BodegaFS lo lee y lo escribe
// completo con leerArchivo y escribirArchivo
class Filesystem {
public:
    Filesystem(const std::string& ruta);
    ~Filesystem();

    void crearArchivo(const std::string& nombreNegocio);

    // revisa si fs.dat.ci0123 existe y trae la firma de FIRMA_CAFETERIA
    bool archivoValido();

    // acceso generico a un bloque de 256 bytes
    void leerBloque(int numBloque, void* destino);
    void escribirBloque(int numBloque, const void* origen);

    int pedirBloque();
    void liberarBloque(int numBloque);
    int bloquesLibres();

    // i-nodos. estas funciones no toman el mutex: quien las llama ya debe tenerlo
    void leerInodo(uint16_t numInodo, Inodo& inodo);
    std::string leerArchivo(uint16_t numInodo);
    bool escribirArchivo(uint16_t numInodo, const std::string& contenido);

    // directorio de bodegas. devuelve el i-nodo de la bodega nueva, o BLOQUE_NULO si no hay espacio
    uint16_t registrarBodega(const std::string& nombre);
    bool buscarBodega(const std::string& nombre, EntradaDirectorio& resultado);
    std::vector<EntradaDirectorio> listarBodegas();

    // quita la bodega del directorio y libera su i-nodo con todos sus bloques de datos
    bool eliminarBodega(const std::string& nombre);

    void leerSuperbloque(Superbloque& sb);

    // BodegaFS envuelve sus propias operaciones con este mutex, porque todas
    // las bodegas comparten el mismo archivo y el mismo bitmap
    std::mutex& mutex();

private:
    std::string ruta_;
    std::fstream archivo_;
    Bitmap bitmap_;
    std::mutex mutex_;

    void abrir();
    void escribirSuperbloque(const Superbloque& sb);

    void escribirInodo(uint16_t numInodo, const Inodo& inodo);
    uint16_t crearInodo(uint8_t tipo);
    void liberarInodo(uint16_t numInodo);

    std::vector<EntradaDirectorio> leerDirectorio();
    bool escribirDirectorio(const std::vector<EntradaDirectorio>& entradas);
};

#endif
