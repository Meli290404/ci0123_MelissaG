/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   IPv4 TCP client normal or SSL according to parameters
  *
  * (Fedora version)
  *
 **/

#include <stdio.h>
#include <string.h>

#include "VSocket.h"
#include "Socket.h"
#include "SSLSocket.h"

#define	MAXBUF	1024

/**
**/
int main( int argc, char * argv[] ) {
   VSocket * client;
   int st, port = 80;
   char a[ MAXBUF ];
   char * os = (char *) "163.178.104.62";
   char * orchid = (char *) "GET /lego/list.php?figure=orchid&part=1 \r\nHTTP/v1.1\r\nhost: redes.ecci\r\n\r\n";
//   char * request = (char *) "GET /ci0123 HTTP/1.1\r\nhost:redes.ecci\r\n\r\n";

   if (argc > 1 ) {
      port = 443;
      client = new SSLSocket();	// Create a new stream socket for IPv4
   } else {
      client = new Socket( 's' );
      port = 80;
   }

   memset( a, 0 , MAXBUF );
   client->Connect( os, port );
/***
   // Prueba explícita de que sí hubo cifrado (solo tiene sentido en modo SSL)
if ( argc > 1 ) {
   SSLSocket * secure = (SSLSocket *) client;
   printf( "Cipher usado: %s\n", secure->GetCipher() );
   secure->ShowCerts();
}
*/
   client->Write(  (char * ) orchid, strlen( orchid ) );
   st = client->Read( a, MAXBUF );
   printf( "Bytes read %d\n%s\n", st, a);

}