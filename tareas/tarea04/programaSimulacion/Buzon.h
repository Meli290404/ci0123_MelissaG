#ifndef BUZON_H
#define BUZON_H

#include <sys/types.h>

#define KEY 0xB54322     // llave para identificar la cola de mensajes
#define TAM_TEXTO 256    // tamaño máximo del texto de un mensaje

// Clase para mandar y recibir mensajes entre procesos usando colas de
// mensajes de System V (msgget, msgsnd, msgrcv).
class Buzon {
public:
    Buzon();
    ~Buzon();

    int Enviar( const char *mensaje, long tipo = 1 );
    int Enviar( const void *mensaje, int cantidad, long tipo = 1 );
    int Recibir( void *mensaje, int cantidad, long tipo = 1 );

private:
    int id;        // id de la cola de mensajes
    pid_t owner;   // pid del proceso que creó la cola (para saber quién la borra)
};

#endif