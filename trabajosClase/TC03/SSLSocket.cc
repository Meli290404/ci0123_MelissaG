/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  SSL Socket class implementation
  *
  * (Fedora version)
  *
 **/

// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdexcept>
#include <string>
#include <cstring>

#include "SSLSocket.h"

/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( bool IPv6 ) {

   this->Init( 's', IPv6 );		// SSL solo tiene sentido sobre TCP (stream)

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL();					// Initializes to client context

}


/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool IPv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( char * certFileName, char * keyFileName, bool IPv6 ) {

   // Constructor para el lado servidor (carga certificado propio).
   // Se completará en la fase de servidor SSL; no forma parte de esta etapa.

}


/**
  *  Class constructor
  *
  *  @param     int id: socket descriptor
  *
 **/
SSLSocket::SSLSocket( int id ) {

   // Reservado para el lado servidor: envolver un socket ya aceptado
   // (accept) en un SSLSocket. VSocket no expone un Init(int), así que
   // por ahora solo se guardan los datos mínimos para que la clase
   // compile; el handshake SSL de servidor se hará en esa fase.
   this->sockId = id;
   this->type = 's';
   this->IPv6 = false;
   this->port = 0;

   this->Context = nullptr;
   this->BIO = nullptr;

}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

// SSL destroy
   if ( nullptr != this->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
   }
   if ( nullptr != this->BIO ) {
      SSL_free( reinterpret_cast<SSL *>( this->BIO ) );
   }

   this->Close();

}


/**
  *  InitSSL
  *     use SSL_new with a defined context
  *
  *  Create a SSL object
  *
 **/
void SSLSocket::InitSSL( bool serverContext ) {

   this->InitContext( serverContext );		// primero necesitamos un contexto (SSL_CTX) válido

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );

   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::InitSSL, SSL_new" );
   }

   this->BIO = (void *) ssl;		// aquí guardamos el objeto SSL (canal seguro) de la instancia

}


/**
  *  InitContext
  *     use SSL_library_init, OpenSSL_add_all_algorithms, SSL_load_error_strings, TLS_server_method, SSL_CTX_new
  *
  *  Creates a new SSL server context to start encrypted comunications, this context is stored in class instance
  *
 **/
void SSLSocket::InitContext( bool serverContext ) {
   const SSL_METHOD * method;
   SSL_CTX * context;

   // El "método" le dice a OpenSSL qué rol y qué versiones de TLS negociar.
   // En versiones recientes de OpenSSL (1.1.0+) ya no hace falta llamar
   // SSL_library_init ni SSL_load_error_strings a mano; se inicializan solas.
   if ( serverContext ) {
      method = TLS_server_method();
   } else {
      method = TLS_client_method();
   }

   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool )" );
   }

   context = SSL_CTX_new( method );
   if ( nullptr == context ) {
      throw std::runtime_error( "SSLSocket::InitContext, SSL_CTX_new" );
   }

   this->Context = (void *) context;

}


/**
 *  Load certificates
 *    verify and load certificates
 *
 *  @param	const char * certFileName, file containing certificate
 *  @param	const char * keyFileName, file containing keys
 *
 **/
 void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName ) {

   // Necesario solo para el lado servidor (fase posterior de este proyecto).

}


/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	int port, service number
 *
 **/
int SSLSocket::Connect( const char * hostName, int port ) {
   int st;

   st = this->TryToConnect( hostName, port );		// Establish a non ssl connection first

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   // Le decimos al objeto SSL sobre qué descriptor de socket (ya conectado
   // por TCP) debe hacer el handshake.
   SSL_set_fd( ssl, this->sockId );

   st = SSL_connect( ssl );		// aquí ocurre el "apretón de manos" (handshake) TLS
   if ( 1 != st ) {
      int sslErr = SSL_get_error( ssl, st );
      throw std::runtime_error( "SSLSocket::Connect, SSL_connect error code "
                                 + std::to_string( sslErr ) );
   }

   return st;

}


/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	char * service, service name
 *
 **/
int SSLSocket::Connect( const char * host, const char * service ) {
   int st;

   st = this->TryToConnect( host, service );		// misma idea, pero resolviendo el puerto por nombre de servicio (ej. "https")

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   SSL_set_fd( ssl, this->sockId );

   st = SSL_connect( ssl );
   if ( 1 != st ) {
      int sslErr = SSL_get_error( ssl, st );
      throw std::runtime_error( "SSLSocket::Connect, SSL_connect error code "
                                 + std::to_string( sslErr ) );
   }

   return st;

}


/**
  *  Read
  *     use SSL_read to read data from an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity read
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Read( void * buffer, size_t size ) {

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   int st = SSL_read( ssl, buffer, (int) size );

   if ( st < 0 ) {
      int sslErr = SSL_get_error( ssl, st );
      throw std::runtime_error( "SSLSocket::Read, SSL_read error code "
                                 + std::to_string( sslErr ) );
   }

   return (size_t) st;		// SSL_read devuelve 0 cuando el otro lado cerró el canal, igual que read()

}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Writes data to a secure channel
  *
 **/
size_t SSLSocket::Write( const char * text ) {

   return this->Write( (const void *) text, strlen( text ) );

}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	const void * buffer to store data to write
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Write( const void * buffer, size_t size ) {

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   int st = SSL_write( ssl, buffer, (int) size );

   if ( st <= 0 ) {
      int sslErr = SSL_get_error( ssl, st );
      throw std::runtime_error( "SSLSocket::Write, SSL_write error code "
                                 + std::to_string( sslErr ) );
   }

   return (size_t) st;

}


/**
 *   Show SSL certificates
 *
 **/
void SSLSocket::ShowCerts() {
   X509 *cert;
   char *line;

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   cert = SSL_get_peer_certificate( ssl );		 // Get certificates (if available)
   if ( nullptr != cert ) {
      printf("Server certificates:\n");
      line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }

}


/**
 *   Return the name of the currently used cipher
 *
 **/
const char * SSLSocket::GetCipher() {

   return SSL_get_cipher( reinterpret_cast<SSL *>( this->BIO ) );

}