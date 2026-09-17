/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** SSLSocket example, server code (fork)
  *
  * (Fedora version)
  *
 **/

#include <cstdlib>	// atoi, exit
#include <cstdio>	// printf, perror
#include <cstring>	// strlen
#include <unistd.h>	// fork

#include "SSLSocket.h"

#define PORT	4321


int main( int cuantos, char ** argumentos ) {
   SSLSocket * server, * client;
   int port = PORT;
   int childpid;
   char buf[ 1024 ];

   if ( cuantos > 1 ) {
      port = atoi( argumentos[ 1 ] );
   }

   server = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", false );
   server->Bind( port );
   server->MarkPassive( 10 );

   for( ; ; ) {
      client = (SSLSocket *) server->AcceptConnection();  // TCP accept, sin SSL todavia
      client->CopyContext( server );                       // hereda certificado del servidor

      childpid = fork();
      if ( childpid < 0 ) {
         perror( "server: fork error" );
      } else if ( 0 == childpid ) {          // codigo del hijo

         server->Close();                    // el hijo no necesita el socket que escucha
         memset( buf, 0, sizeof( buf ) );

         client->Accept();                   // handshake SSL, AHORA si (dentro del hijo, con su propio SSL*)
         client->ShowCerts();

         client->Read( buf, sizeof( buf ) );  // SSL_read
         printf( "Server (child pid %d) received: \"%s\"\n", getpid(), buf );
         client->Write( buf );                // SSL_write (mirror)

         exit( 0 );
      }

      delete client;    // en el papa, ya no lo necesita: el hijo tiene su propia copia del descriptor via fork()

   }

}