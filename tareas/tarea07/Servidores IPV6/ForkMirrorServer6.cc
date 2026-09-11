/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   Socket client/server example (fork) - version IPv6
  *
  * (Fedora version)
  *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h> // memset
#include <unistd.h>
#include <iostream>

#include "Socket.h"

#define PORT 2026
#define BUFSIZE 512

int main( int argc, char ** argv ) {
   VSocket * s1, * s2;
   int childpid;
   char a[ BUFSIZE ];

   // el 'true' aqui es la unica diferencia real contra la version IPv4:
   // le dice al constructor que arme un socket AF_INET6 en vez de AF_INET.
   s1 = new Socket( 's', true );   // Create a stream IPv6 socket

   s1->Bind( PORT );            // Port to access this mirror server (usa sockaddr_in6 por dentro)
   s1->MarkPassive( 5 );        // Set passive socket and backlog queue to 5 connections

   for( ; ; ) {
      s2 = s1->AcceptConnection();  // Wait for a new connection; s2 ya viene marcado como IPv6 tambien
      childpid = fork();        // Create a child to serve the request
      if ( childpid < 0 ) {
         perror( "server: fork error" );
      } else {
         if (0 == childpid) {       // child code
            s1->Close();            // Close original socket "s1" in child
            memset( a, 0, BUFSIZE );
            s2->Read( a, BUFSIZE ); // Read a string from client using new conection info
            std::cout << "Server IPv6 (child pid " << getpid() << ") received: " << a << std::endl;
            s2->Write( a );     // Write it back to client, this is the mirror function
            exit( 0 );          // Exit, finish child work
         }
      }

      s2->Close();          // Close socket s2 in parent, then go wait for a new conection

   }

}