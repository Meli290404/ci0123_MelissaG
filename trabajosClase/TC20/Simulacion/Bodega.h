#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>

#include "Buzon.h"
#include "Protocolo.h"
#include "FileSystem.h"
#include "BodegaFS.h"

// hilo que hace el papel de una bodega. sus productos ya no viven en memoria:
// son las lineas de su archivo de texto dentro del filesystem de la isla
class Bodega {
public:
    // idIntermediario e idTextoIntermediario son los del intermediario de la isla a la que pertenece.
    // nombre es tambien el nombre de la bodega en el directorio del filesystem; si no esta ahi
    // se lanza std::runtime_error
    Bodega( Buzon & buzon, int miId, const std::string & nombre,
             int idIntermediario, const std::string & idTextoIntermediario,
             Filesystem & fs, const std::string & rutaArchivo );

    void ejecutar();

private:
    Buzon & buzon_;
    int miId_;
    std::string nombre_;
    int idInt_;
    std::string txtInt_;
    Filesystem & fs_;
    std::string rutaArchivo_;
    uint16_t inodo_;
    BodegaFS archivo_;   // archivo de texto de esta bodega dentro del filesystem

    void anunciarse();
    void atenderCategorias();
    void atenderProductos( const std::string & categoria );
    void atenderReserva( const std::string & nombre, const std::string & cantidadTexto );

    // envia la respuesta al intermediario y la rotula
    void responder( const std::string & msg, const std::string & explicacion );
    std::string listaCategorias();

    // rotula un acceso al filesystem con el estado actual del i-nodo de la bodega
    void rotularAcceso( const std::string & accion );
};

#endif
