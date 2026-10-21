// prueba de los metodos de broadcast de VSocket
//
// receptor: se queda esperando datagramas enviados a la direccion broadcast
//   ./pruebaBroadcast receptor <ipBroadcast> <puerto>
//
// emisor: envia un mensaje a la direccion broadcast
//   ./pruebaBroadcast emisor <ipBroadcast> <puerto> <mensaje>
//
// calcular la direccion broadcast de una red
//   ./pruebaBroadcast calcular <ip> <prefijo>

#include <iostream>
#include <string>
#include <stdexcept>

#include "Socket.h"

static void mostrarUso( const char * programa ) {
    std::cerr << "uso:\n"
              << "  " << programa << " receptor <ipBroadcast> <puerto>\n"
              << "  " << programa << " emisor <ipBroadcast> <puerto> <mensaje>\n"
              << "  " << programa << " calcular <ip> <prefijo>\n";
}

// el receptor hace bind a la direccion broadcast: si lo hiciera a su propia ip
// el sistema no le entregaria los datagramas enviados a la broadcast
static void receptor( const char * ipBroadcast, int puerto ) {
    Socket socket( 'd' );
    socket.Bind( ipBroadcast, puerto );
    std::cout << "[receptor] esperando en " << ipBroadcast << ":" << puerto << " (Ctrl+C para terminar)" << std::endl;

    char buffer[ 1024 ];
    while ( true ) {
        std::string ipOrigen;
        int puertoOrigen;
        size_t leidos = socket.recvFrom( buffer, sizeof( buffer ) - 1, ipOrigen, puertoOrigen );
        buffer[ leidos ] = '\0';
        std::cout << "[receptor] de " << ipOrigen << ":" << puertoOrigen << " -> " << buffer << std::endl;
    }
}

static void emisor( const char * ipBroadcast, int puerto, const std::string & mensaje ) {
    Socket socket( 'd' );
    socket.EnableBroadcast();
    size_t enviados = socket.sendTo( mensaje.data(), mensaje.size(), ipBroadcast, puerto );
    std::cout << "[emisor] " << enviados << " bytes enviados a " << ipBroadcast << ":" << puerto << "\n";
}

int main( int argc, char * argv[] ) {
    if ( argc < 4 ) {
        mostrarUso( argv[ 0 ] );
        return 1;
    }
    std::string modo = argv[ 1 ];

    try {
        if ( "receptor" == modo ) {
            receptor( argv[ 2 ], std::stoi( argv[ 3 ] ) );
        } else if ( "emisor" == modo && argc >= 5 ) {
            emisor( argv[ 2 ], std::stoi( argv[ 3 ] ), argv[ 4 ] );
        } else if ( "calcular" == modo ) {
            std::cout << VSocket::BroadcastAddress( argv[ 2 ], std::stoi( argv[ 3 ] ) ) << "\n";
        } else {
            mostrarUso( argv[ 0 ] );
            return 1;
        }
    } catch ( const std::exception & e ) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}