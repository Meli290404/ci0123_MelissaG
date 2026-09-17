/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** SSLSocket example, server code (threads)
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


/**
 *   Task que corre cada hilo nuevo
 *      Hace el handshake SSL (Accept), lee un mensaje y lo regresa (mirror)
 *
 **/
void Service( SSLSocket * client ) {
   char buf[ 1024 ] = { 0 };
   int bytes;

   client->Accept();       // handshake SSL/TLS del lado servidor (SSL_accept), ya con el TCP accept hecho
   client->ShowCerts();    // por curiosidad, imprime los certificados si los hay (en este caso el cliente no manda, va a decir "No certificates")

   bytes = client->Read( buf, sizeof( buf ) );  // SSL_read por dentro
   buf[ bytes ] = '\0';
   printf( "Server received: \"%s\"\n", buf );

   client->Write( buf );   // devolvemos el mismo mensaje (mirror), SSL_write por dentro

   delete client;          // el destructor cierra el socket y libera el SSL* (el contexto NO, porque ownsContext=false)

}


/**
 *   Crea el servidor SSL
 *      Bind + MarkPassive vienen heredados de VSocket, funcionan igual
 *      que en la version sin SSL
 *      Loop infinito: AcceptConnection (TCP) + CopyContext (comparte el
 *      certificado) + un hilo nuevo que hace el handshake SSL y atiende
 *
 **/
int main( int cuantos, char ** argumentos ) {
   SSLSocket * server, * client;
   std::thread * worker;
   int port = PORT;

   if ( cuantos > 1 ) {
      port = atoi( argumentos[ 1 ] );
   }

   // 'false' al final = IPv4. Los .pem hay que generarlos con openssl antes de correr esto
   server = new SSLSocket( (char *) "ci0123.pem", (char *) "key0123.pem", false );
   server->Bind( port );
   server->MarkPassive( 10 );

   for( ; ; ) {
      client = (SSLSocket *) server->AcceptConnection();  // TCP accept "crudo", todavia sin SSL
      client->CopyContext( server );                       // hereda el certificado del servidor (no crea uno nuevo)
      worker = new std::thread( Service, client );          // el handshake SSL (Accept) se hace DENTRO del hilo
   }

}