#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "SSLSocket.h"

int main( int cuantos, char ** argumentos ) {
   SSLSocket * client;
   char buf[ 1024 ] = { 0 };
   int bytes;

   if ( cuantos < 4 ) {
      printf( "uso: %s <ip> <puerto> <mensaje>\n", argumentos[0] );
      exit(0);
   }

   client = new SSLSocket( false );   // constructor cliente, IPv4
   client->Connect( argumentos[1], atoi( argumentos[2] ) );

   printf( "Conectado con cifrado: %s\n", client->GetCipher() );
   client->ShowCerts();

   client->Write( argumentos[3] );
   bytes = client->Read( buf, sizeof(buf) );
   buf[bytes] = '\0';
   printf( "Recibido: \"%s\"\n", buf );

   delete client;
}