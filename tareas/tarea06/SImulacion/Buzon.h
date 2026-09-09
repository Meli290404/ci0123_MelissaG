/**
  *   C++ class to encapsulate Unix message passing intrinsic structures and system calls
  *
  *   UCR-ECCI
  *
  *   CI0122 Sistemas Operativos 2026-ii
  *
  *   Class interface
  *
 **/

#ifndef BUZON_H
#define BUZON_H

#include <sys/types.h>  // pid_t definition

#define KEY 0xB54321     // Valor de la llave del recurso
#define TAM_TEXTO 256    // Tamaño máximo de una línea de protocolo

class Buzon {
public:
    Buzon();
    ~Buzon();
    int Enviar( const char *mensaje, long = 1 );
    int Enviar( const void *mensaje, int, long = 1 );
    int Recibir( void *mensaje, int, long = 1 ); // len: space in mensaje

    // Tiene la misma funcionalidad que Recibir(), pero sin bloquear para siempre.
    // Reintenta "intentos" veces.
    // "esperaMs" espera en milisegundos entre cada intento. Devuelve -1 si nunca llegó
    // el mensaje esperado (se utiliza para simular ERR_COMM del protocolo).
    int RecibirConEspera( void *mensaje, int cantidad, long tipo, int intentos, int esperaMs );

private:
    int id;        // Identificador del buzon
    pid_t owner;   // Mailbox owner
};

#endif