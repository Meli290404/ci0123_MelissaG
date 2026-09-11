/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   Socket client/server example with threads - version IPv6
  *
 **/
#include <iostream>
#include <thread>
#include <cstring>       // memset

#include "Socket.h"

#define PORT 2026
#define BUFSIZE 512


/**
 *   Task each new thread will run
 *      Read string from socket
 *      Write it back to client
 *
 **/
void task( VSocket * client ) {
   char a[ BUFSIZE ];
   memset( a, 0, BUFSIZE ); // limpiar el buffer antes de leer, si no queda basura de memoria

   client->Read( a, BUFSIZE );  // Read a string from client, data will be limited by BUFSIZE bytes
   std::cout << "Server IPv6 received: " << a << std::endl;
   client->Write( a );      // Write it back to client, this is the mirror function
   client->Close();     // Close socket in parent

}


/**
 *   Create server code
 *      Infinite for
 *         Wait for client conection
 *         Spawn a new thread to handle client request
 *
 **/
int main( int argc, char ** argv ) {
   std::thread * worker;
   VSocket * s1, * client;

   // 'true' arma un socket AF_INET6; Bind() y AcceptConnection() ya
   // saben internamente que hacer con eso gracias a this->IPv6
   s1 = new Socket( 's', true );

   s1->Bind( PORT );        // Port to access this mirror server
   s1->MarkPassive( 5 );    // Set socket passive and backlog queue to 5 connections

   for( ; ; ) {
      client = s1->AcceptConnection();      // Wait for a client connection (queda marcado IPv6 tambien)
      worker = new std::thread( task, client );
   }

}