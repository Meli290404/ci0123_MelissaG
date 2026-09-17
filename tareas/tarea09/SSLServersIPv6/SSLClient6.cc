/**
  *   Cliente SSL de prueba - version IPv6
  *
  *   Deben usar la direccion IPv6 del equipo donde corre el servidor.
  *   Para pruebas en la misma maquina, "::1" es el equivalente a
  *   127.0.0.1 (localhost en IPv6).
  *
 **/
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "SSLSocket.h"

int main( int cuantos, char ** argumentos ) {
   SSLSocket * client;
   char buf[ 1024 ] = { 0 };
   int bytes;

   if ( cuantos < 4 ) {
      printf( "uso: %s <ipv6> <puerto> <mensaje>\n", argumentos[0] );
      printf( "ejemplo: %s ::1 4433 \"pollo ipv6\"\n", argumentos[0] );
      exit(0);
   }

   client = new SSLSocket( true );   // constructor cliente, 'true' = IPv6
   client->Connect( argumentos[1], atoi( argumentos[2] ) );

   printf( "Conectado con cifrado: %s\n", client->GetCipher() );
   client->ShowCerts();

   client->Write( argumentos[3] );
   bytes = client->Read( buf, sizeof(buf) );
   buf[bytes] = '\0';
   printf( "Recibido: \"%s\"\n", buf );

   delete client;
}