/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   Socket client/server example - version IPv6
  *
  *   Deben usar la direccion IPv6 del equipo donde va a correr el servidor.
  *   Para pruebas en la misma maquina, "::1" es el equivalente a 127.0.0.1
  *   (localhost en IPv6).
  *
 **/

#include <stdio.h>
#include <cstring>
#include "Socket.h"

#define PORT 2026
#define BUFSIZE 512

int main( int argc, char ** argv ) {
   VSocket * s;
   char buffer[ BUFSIZE ];

   // 'true' le dice al constructor que arme un socket AF_INET6; TryToConnect
   // ya sabe (por this->IPv6) que debe armar un sockaddr_in6 e inet_pton con
   // AF_INET6 en vez de AF_INET, eso ya estaba resuelto en VSocket::TryToConnect
   s = new Socket( 's', true );     // Create a new stream IPv6 socket
   memset( buffer, 0, BUFSIZE );    // Zero fill buffer

   s->Connect( "::1", PORT );  // "::1" = localhost en IPv6. Cambiar por la IPv6 real si es entre 2 PCs
   if ( argc > 1 ) {
      s->Write( argv[1] );      // If provided, send first program argument to server
   } else {
      s->Write( "Hello world 2026 IPv6..." );
   }
   s->Read( buffer, BUFSIZE );  // Read answer sent back from server
   printf( "%s\n", buffer );    // Print received string, mirror example this will print same sent string

}