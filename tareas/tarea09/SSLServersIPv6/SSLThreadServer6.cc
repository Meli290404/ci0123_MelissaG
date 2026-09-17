/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** SSLSocket example, server code (threads) - version IPv6
  *
  * (Fedora version)
  *
 **/

#include <thread>
#include <cstdlib>	// atoi
#include <cstdio>	// printf
#include <cstring>	// strlen, strcmp

#include "SSLSocket.h"

#define PORT	4321


void Service( SSLSocket * client ) {
   char buf[ 1024 ] = { 0 };
   int bytes;

   client->Accept();       // handshake SSL/TLS del lado servidor
   client->ShowCerts();

   bytes = client->Read( buf, sizeof( buf ) );
   buf[ bytes ] = '\0';
   printf( "Server IPv6 received: \"%s\"\n", buf );

   client->Write( buf );   // mirror, cifrado

   delete client;

}


int main( int cuantos, char ** argumentos ) {
   SSLSocket * server, * client;
   std::thread * worker;
   int port = PORT;

   if ( cuantos > 1 ) {
      port = atoi( argumentos[ 1 ] );
   }

   // 'true' al final = IPv6. Es la UNICA diferencia real contra la
   // version IPv4: el resto (Bind, MarkPassive, AcceptConnection,
   // CopyContext, Accept) ya sabe que hacer gracias a this->IPv6,
   // que SSLSocket hereda de VSocket y propaga en todos lados
   server = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", true );
   server->Bind( port );        // ahora arma un sockaddr_in6 por dentro
   server->MarkPassive( 10 );

   for( ; ; ) {
      client = (SSLSocket *) server->AcceptConnection();  // TCP accept, sin SSL todavia; hereda IPv6=true
      client->CopyContext( server );                       // comparte el certificado del servidor
      worker = new std::thread( Service, client );          // el handshake SSL se hace dentro del hilo
   }

}