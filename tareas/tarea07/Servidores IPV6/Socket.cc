/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  ******   Socket class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>         // sockaddr_in
#include <arpa/inet.h>          // ntohs
#include <unistd.h>     // write, read
#include <cstring>
#include <stdexcept>
#include <stdio.h>      // printf

#include "Socket.h"     // Derived class

/**
  *  Class constructor
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
Socket::Socket( char t, bool IPv6 ){

   this->Init( t, IPv6 );      // Call base class constructor

}


/**
  *  Class constructor (desde un descriptor ya existente)
  *
  *  Este es el constructor "chiquito" que se usa cuando ya tenemos un
  *  file descriptor hecho (por ejemplo el que devuelve accept() dentro
  *  de WaitForConnection). En vez de crear un socket nuevo desde cero,
  *  solo "se envuelve" el descriptor que ya existe en un objeto Socket
  *  para poder usar Read/Write/Close sobre el normalmente.
  *
  *  @param     int id: socket descriptor
  *  @param     bool ipv6: si ese descriptor viene de un socket IPv6
  *     (AcceptConnection nos pasa this->IPv6 del socket que escuchaba,
  *      para que el hijo/hilo que atiende sepa que familia usar)
  *
 **/
Socket::Socket( int id, bool ipv6 ) {

   this->Init( id, ipv6 );      // Call base class Init(int, bool), que solo adopta el descriptor

}


/**
  *  Class destructor
  *
 **/
Socket::~Socket() {

}


/**
  * Connect method
  *   use "TryToConnect" in base class
  *
  * @param      char * host: host address in dot notation, example "10.1.166.62"
  * @param      int port: process address, example 80
  *
 **/
int Socket::Connect( const char * hostip, int port ) {

   return this->TryToConnect( hostip, port );

}


/**
  * Connect method
  *   use "TryToConnect" in base class
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int Socket::Connect( const char *host, const char *service ) {

   return this->TryToConnect( host, service );

}


/**
  * Read method
  *   use "read" Unix system call (man 2 read)
  *
  * @param      void * buffer: buffer to store data read from socket
  * @param      int size: buffer capacity, read will stop if buffer is full
  *
 **/
size_t Socket::Read( void * buffer, size_t size ) {

   //leer socket como si fuera archivo
   ssize_t st = read( this->sockId, buffer, size );

   if ( -1 == st ) {
      throw std::runtime_error( "Socket::Read failed" );
   }

   return st;

}


/**
  * Write method
  *   use "write" Unix system call (man 2 write)
  *
  * @param      void * buffer: buffer to store data write to socket
  * @param      size_t size: buffer capacity, number of bytes to write
  *
 **/
size_t Socket::Write( const void * buffer, size_t size ) {

   //enviar datos binarios por el socket
   ssize_t st = write( this->sockId, buffer, size );

   if ( -1 == st ) {
      perror( "write failed" );
      throw std::runtime_error( "Socket::Write failed" );
   }

   return st;

}


/**
  * Write method
  *   use "write" Unix system call (man 2 write)
  *
  * @param      char * text: text to write to socket
  *
 **/
size_t Socket::Write( const char * text ) {

   //enviar datos de texto por el socket
   return Write( (const void *) text, strlen( text ) );

}

/**
  * AcceptConnection method
  *    use base class to accept connections
  *
  *  @returns   a new class instance
  *
  *  Waits for a new connection to service (TCP mode: stream)
  *
  *  Flujo: WaitForConnection() llama a accept() y bloquea el proceso hasta
  *  que llega un cliente; cuando eso pasa devuelve un descriptor nuevo (id).
  *  Con ese id se crea un Socket nuevo, pasandole this->IPv6 para que sepa
  *  si es un socket IPv4 o IPv6. Ese objeto "peer"es con el que de ahi en adelante
  *  hablamos con ese cliente en particular (Read/Write), mientras el socket original (this)
  *  se queda libre para seguir escuchando.
  *
 **/
VSocket * Socket::AcceptConnection(){
   int id;
   VSocket * peer;

   id = this->WaitForConnection();

   peer = new Socket( id, this->IPv6 );   // propagamos la familia del socket que escucha

   return peer;
}