#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "Buzon.h"

// mtype tiene que ir siempre de primero en el struct, eso lo exige msgsnd/msgrcv.
struct MsgBuf {
    long mtype;
    char texto[ TAM_TEXTO ];
};

Buzon::Buzon() {
    // Se intenta crear la cola con IPC_EXCL para saber si yo la cree.
    // Si ya existe (EEXIST), me conecto a la que ya hay.
    int st = msgget( KEY, IPC_CREAT | IPC_EXCL | 0666 );
    bool creador = true;

    if ( -1 == st ) {
        if ( errno == EEXIST ) {
            creador = false;
            st = msgget( KEY, 0666 );
        }
        if ( -1 == st ) {
            throw std::runtime_error( "Buzon::Buzon(): no se pudo crear/conectar la cola" );
        }
    }

    id = st;
    // Si no fui yo quien la creo, dejo owner en -1 para no borrarla despues.
    owner = creador ? getpid() : -1;
}

Buzon::~Buzon() {
    // Solo borro la cola si fui yo quien la creo. Si cada proceso que se
    // conecta tambien la borrara, el que termine primero se la quitaria
    // al otro mientras todavia la esta usando.
    if ( owner != getpid() ) {
        return;
    }

    int st = msgctl( id, IPC_RMID, nullptr );
    if ( -1 == st && errno != EINVAL ) {
        fprintf( stderr, "Buzon::~Buzon(): no se pudo borrar la cola (%s)\n", strerror( errno ) );
    }
}

// Manda un mensaje de texto por la cola.
int Buzon::Enviar( const char * mensaje, long tipo ) {
    MsgBuf msg;
    msg.mtype = tipo;

    strncpy( msg.texto, mensaje, TAM_TEXTO - 1 );
    msg.texto[ TAM_TEXTO - 1 ] = '\0';

    size_t tam = strlen( msg.texto ) + 1;
    int st = msgsnd( id, &msg, tam, 0 );

    if ( -1 == st ) {
        throw std::runtime_error( "Buzon::Enviar(): fallo al enviar" );
    }
    return st;
}

// Manda un bloque de bytes cualquiera (no solo texto).
int Buzon::Enviar( const void * mensaje, int cantidad, long tipo ) {
    char * buffer = new char[ sizeof( long ) + cantidad ];
    *reinterpret_cast<long *>( buffer ) = tipo;
    memcpy( buffer + sizeof( long ), mensaje, cantidad );

    int st = msgsnd( id, buffer, cantidad, 0 );
    delete [] buffer;

    if ( -1 == st ) {
        throw std::runtime_error( "Buzon::Enviar(): fallo al enviar" );
    }
    return st;
}

// Se bloquea hasta recibir un mensaje del tipo pedido.
int Buzon::Recibir( void * mensaje, int cantidad, long tipo ) {
    int st = msgrcv( id, mensaje, cantidad, tipo, 0 );

    if ( -1 == st ) {
        throw std::runtime_error( "Buzon::Recibir(): fallo al recibir" );
    }
    return st;
}