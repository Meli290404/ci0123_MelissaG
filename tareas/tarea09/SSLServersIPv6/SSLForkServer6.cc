/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** SSLSocket example, server code (fork) - version IPv6
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

   server = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", true );  // 'true' = IPv6
   server->Bind( port );
   server->MarkPassive( 10 );

   for( ; ; ) {
      client = (SSLSocket *) server->AcceptConnection();  // hereda IPv6=true del server
      client->CopyContext( server );

      childpid = fork();
      if ( childpid < 0 ) {
         perror( "server: fork error" );
      } else if ( 0 == childpid ) {

         server->Close();
         memset( buf, 0, sizeof( buf ) );

         client->Accept();       // handshake SSL, ya dentro del hijo
         client->ShowCerts();

         client->Read( buf, sizeof( buf ) );
         printf( "Server IPv6 (child pid %d) received: \"%s\"\n", getpid(), buf );
         client->Write( buf );

         exit( 0 );
      }

      delete client;

   }

}