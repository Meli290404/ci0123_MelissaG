/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *******   VSocket base class implementation
  *
  * (Fedora version)
  *
 **/
 
#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons, inet_pton
#include <netinet/in.h>	// sockaddr_in, INADDR_ANY
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>		// getaddrinfo, freeaddrinfo
#include <unistd.h>		// close
#include <cerrno>      // errno
 
#include "VSocket.h"
 
 
/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::Init( char t, bool IPv6 ){
   this->IPv6 = IPv6;
   this->type = t;  
   this->port = 0;
 
   int st; //socket type
 
   if (t == 's') {
      st = SOCK_STREAM;
   }
   else if (t == 'd') {
      st = SOCK_DGRAM;
   }
   else {
      throw std::runtime_error( "VSocket::Init, invalid socket type" );
   }
 
   int domain = IPv6 ? AF_INET6 : AF_INET;
   this->sockId = socket( domain, st, 0 );
 
 
   if ( -1 == this->sockId ) {
      throw std::runtime_error( "VSocket::Init, (reason)" );
   }
 
}
 
 
/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {
 
   this->Close();
 
}
 
 
/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close(){
 
   if (this->sockId != -1) {
      int st = close( this->sockId );
      
      if ( -1 == st ) {
      throw std::runtime_error( "VSocket::Close()" );
   }
 
   this->sockId = -1;
 
   }
 
}
 
 
/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::TryToConnect( const char * hostip, int port ) {
 
   
   int st = -1;
 
   if ( !IPv6 ) {
 
      struct sockaddr_in host4;
      memset( (char *) &host4, 0, sizeof( host4 ) );
      host4.sin_family = AF_INET;
 
      st = inet_pton( AF_INET, hostip, &host4.sin_addr );
      if ( 1 != st ) {
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton: direccion IPv4 invalida" );
      }
 
      host4.sin_port = htons( (uint16_t) port );
 
      st = connect( sockId, (sockaddr *) &host4, sizeof( host4 ) );
      if ( -1 == st ) {
         throw std::runtime_error( std::string( "VSocket::TryToConnect, connect: " ) + strerror( errno ) );
      }
 
   } else {
 
      struct sockaddr_in6 host6;
      memset( (char *) &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;
 
      // NOTA: inet_pton no entiende el sufijo "%interfaz" de las direcciones
      // link-local (fe80::...%eno1). Para esas direcciones usar la version
      // TryToConnect( host, service ), que si lo soporta via getaddrinfo.
      st = inet_pton( AF_INET6, hostip, &host6.sin6_addr );
      if ( 1 != st ) {
         throw std::runtime_error( "VSocket::TryToConnect, inet_pton: direccion IPv6 invalida "
                                    "(si es link-local con %interfaz use TryToConnect(host, service))" );
      }
 
      host6.sin6_port = htons( (uint16_t) port );
 
      st = connect( sockId, (sockaddr *) &host6, sizeof( host6 ) );
      if ( -1 == st ) {
         throw std::runtime_error( std::string( "VSocket::TryToConnect, connect: " ) + strerror( errno ) );
      }
 
   }
 
   this->port = port;
   return st;
 
}
 
 
/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/
int VSocket::TryToConnect( const char *host, const char *service ) {
   struct addrinfo hints;
   struct addrinfo * res = nullptr;
   struct addrinfo * rp  = nullptr;
 
   memset( &hints, 0, sizeof( hints ) );
   hints.ai_family   = IPv6 ? AF_INET6 : AF_INET;
   hints.ai_socktype = ( 's' == type ) ? SOCK_STREAM : SOCK_DGRAM;
 
   int st = getaddrinfo( host, service, &hints, &res );
   if ( 0 != st ) {
      throw std::runtime_error( std::string( "VSocket::TryToConnect, getaddrinfo: " ) + gai_strerror( st ) );
   }
 
   int connected = -1;
   for ( rp = res; nullptr != rp; rp = rp->ai_next ) {
      connected = connect( sockId, rp->ai_addr, rp->ai_addrlen );
      if ( -1 != connected ) {
         break;   // conexion exitosa con esta direccion
      }
   }
 
   freeaddrinfo( res );
 
   if ( -1 == connected ) {
      throw std::runtime_error( std::string( "VSocket::TryToConnect, connect: " ) + strerror( errno ) );
   }
 
   return st;
 
}
 
/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind( int port ) {
   int st = -1;
 
   // Por ahora solo IPv4 (mas adelante se agrega la rama IPv6)
   if ( !IPv6 ) {
 
      struct sockaddr_in host4;
      memset( (char *) &host4, 0, sizeof( host4 ) );
 
      host4.sin_family = AF_INET;
      host4.sin_addr.s_addr = htonl( INADDR_ANY );  // escuchar en todas las interfaces de esta maquina
      host4.sin_port = htons( (uint16_t) port );     // puerto en network byte order
      memset( host4.sin_zero, '\0', sizeof( host4.sin_zero ) );
 
      st = bind( sockId, (struct sockaddr *) &host4, sizeof( host4 ) );
      if ( -1 == st ) {
         throw std::runtime_error( std::string( "VSocket::Bind, bind: " ) + strerror( errno ) );
      }
 
   } else {
      throw std::runtime_error( "VSocket::Bind, IPv6 aun no implementado" );
   }
 
   this->port = port;
   return st;
 
}
 
 
/**
  *  sendTo method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to send data
  *
  *  Send data to another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {
 
   // addr ya viene armado por quien llama (family, IP y puerto de destino)
   struct sockaddr_in * dest = (struct sockaddr_in *) addr;
 
   ssize_t st = sendto( sockId, buffer, size, 0,
                         (struct sockaddr *) dest, sizeof( struct sockaddr_in ) );
 
   if ( -1 == st ) {
      throw std::runtime_error( std::string( "VSocket::sendTo, sendto: " ) + strerror( errno ) );
   }
 
   return (size_t) st;
 
}
 
 
/**
  *  recvFrom method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to receive from data
  *
  *  @return	size_t bytes received
  *
  *  Receive data from another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {
 
   struct sockaddr_in * from = (struct sockaddr_in *) addr;
   socklen_t fromLen = sizeof( struct sockaddr_in );
 
   // recvfrom llena "from" con la direccion de quien nos escribio
   ssize_t st = recvfrom( sockId, buffer, size, 0,
                           (struct sockaddr *) from, &fromLen );
 
   if ( -1 == st ) {
      throw std::runtime_error( std::string( "VSocket::recvFrom, recvfrom: " ) + strerror( errno ) );
   }
 
   return (size_t) st;
 
}
 
