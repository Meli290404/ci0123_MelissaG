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
#include <cstring>    // strlen
#include <cstdio>     // fprintf, printf

#include "SSLSocket.h"
#include "Socket.h"

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

   this->Init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL();					// Initializes to client context
   this->ownsContext = true;   // este objeto creo su propio contexto, el es quien lo debe liberar

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
   
   this->Init('s', IPv6);
   this->Context = nullptr;
   this->BIO = nullptr;
   this->InitSSL(true);
   this->LoadCertificates(certFileName, keyFileName);
   this->ownsContext = true;   // este es el socket servidor "original", el dueno del contexto con el certificado cargado
}


/**
  *  Class constructor (desde un descriptor ya existente)
  *
  *  Se usa para envolver el descriptor nuevo que entrega accept() (ver
  *  AcceptConnection() mas abajo), o sea para las conexiones que el
  *  servidor va aceptando una por una.
  *
  *  A diferencia de los otros dos constructores, este NO crea un
  *  contexto SSL nuevo (this->Context se deja en nullptr). El contexto
  *  se lo va a "prestar" el socket servidor via CopyContext(), justo
  *  despues de llamar a este constructor (ver AcceptConnection()).
  *
  *  @param     int id: socket descriptor
  *  @param     bool ipv6: si el descriptor es de un socket IPv6 (se
  *     hereda del socket que estaba escuchando, ver AcceptConnection)
  *
 **/
SSLSocket::SSLSocket( int id, bool ipv6 ) {
   this->sockId = id;
   this->IPv6 = ipv6;
   this->port = 0;
   this->type = 's';
   this->Context = nullptr;
   this->BIO = nullptr;
   this->ownsContext = false;  // este objeto NO crea contexto propio, lo va a tomar prestado via CopyContext()
}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

// SSL destroy
   // Solo liberamos el contexto si ESTE objeto fue el que lo creo (ownsContext).
   // Las conexiones aceptadas comparten el mismo SSL_CTX* del servidor (via
   // CopyContext), asi que si cada una lo liberara al cerrarse, estariamos
   // liberando el mismo puntero varias veces -> crash (double free).
   if ( this->ownsContext && nullptr != this->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
   }

   // el SSL * (BIO) en cambio SIEMPRE es propio de cada objeto (cada uno
   // se crea el suyo, ya sea en el constructor o en CopyContext), asi
   // que este si se libera siempre
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
   this->InitContext( serverContext );
   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );
   if(!ssl) {
      throw std::runtime_error( "SSLSocket::InitSSL( SSL_new failed )" );
   }
   this->BIO = reinterpret_cast<void *>( ssl );
}


/**
  *  InitContext
  *     use SSL_library_init, OpenSSL_add_all_algorithms, SSL_load_error_strings, TLS_server_method, SSL_CTX_new
  *
  *  Creates a new SSL server context to start encrypted comunications, this context is stored in class instance
  *
 **/
void SSLSocket::InitContext( bool serverContext ) {
   const SSL_METHOD * method = serverContext ? TLS_server_method() : TLS_client_method();
   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ):method is null");
   }
  
   SSL_CTX * context = SSL_CTX_new( method );
   if (!context) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ) SSL_CTX_new failed" );
   }

   this->Context = reinterpret_cast<void *>( context );

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
   SSL_CTX *ctx = reinterpret_cast<SSL_CTX *>(this->Context);

   if (SSL_CTX_use_certificate_file(ctx, certFileName, SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSLSocket::LoadCertificates(): Error - certificate file");
   }

   if (SSL_CTX_use_PrivateKey_file(ctx, keyFileName, SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSLSocket::LoadCertificates(): Error - key file");
   }

   if (!SSL_CTX_check_private_key(ctx)) {
      throw std::runtime_error("SSLSocket::LoadCertificates(): Private key different to the public certificate");
   }
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
   if (st < 0) return st;

   SSL *ssl = reinterpret_cast<SSL *>( this->BIO );
   SSL_set_fd(ssl, this->sockId);//socket descriptor is attached to SSL

   st = SSL_connect(ssl);//establish a SSL connection
   if (st != 1) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_connect failed with error code %d\n", err);
      return -1;
   }
   return 0;
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

   st = this->TryToConnect( host, service );
   SSL *ssl = reinterpret_cast<SSL *>( this->BIO );
   SSL_set_fd(ssl, this->sockId);//socket descriptor is attached to SSL
   st = SSL_connect(ssl);//establish a SSL connection
   if (st != 1) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_connect failed with error code %d\n", err);
      return -1;
   }
   return 0;

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
   SSL *ssl = reinterpret_cast<SSL *>(this->BIO);
   int st = SSL_read(ssl, buffer, static_cast<int>(size));

   if (st <= 0) {
      int err = SSL_get_error(ssl, st);

      if (err == SSL_ERROR_ZERO_RETURN) {
         return 0;
      }

      fprintf(stderr, "SSL_read error code %d\n", err);
      throw std::runtime_error("SSLSocket::Read(): SSL_read failed");
   }

   return static_cast<size_t>(st);

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
size_t SSLSocket::Write( const char * string ) {
   return this->Write(static_cast<const void *>(string), strlen(string));
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
   SSL *ssl = reinterpret_cast<SSL *>(this->BIO);
   int st = SSL_write(ssl, buffer, static_cast<int>(size));

   if (st <= 0) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_write error code %d\n", err);
      throw std::runtime_error("SSLSocket::Write(): SSL_write failed");
   }

   return static_cast<size_t>(st);

}


/**
 *   Show SSL certificates
 *
 **/
void SSLSocket::ShowCerts() {
   X509 *cert;
   char *line;

   cert = SSL_get_peer_certificate( (SSL *) this->BIO );		 // Get certificates (if available)
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


/**
  *  AcceptConnection
  *     TCP accept "crudo": usa WaitForConnection() (heredado de VSocket,
  *     por dentro hace el accept() de Unix) para esperar un cliente nuevo,
  *     y envuelve el descriptor resultante en un SSLSocket nuevo.
  *
  *  Esto es obligatorio implementarlo: VSocket declara AcceptConnection()
  *  como puro virtual, asi que sin esto SSLSocket queda "abstracta" y ni
  *  siquiera se puede instanciar con "new SSLSocket(...)".
  *
  *  El objeto que devuelve todavia no tiene contexto SSL ni ha hecho
  *  el handshake. Falta llamar CopyContext() (para heredar el certificado
  *  del servidor) y despues Accept() (para el handshake SSL_accept). Asi
  *  se ve en el ejemplo SSLServer.cc:
  *
  *     client = (SSLSocket *) server->AcceptConnection();
  *     client->CopyContext( server );
  *     ... luego, ya en el hilo/proceso hijo: client->Accept();
  *
  *  @returns   un VSocket* que en realidad es un SSLSocket* (hay que
  *     castearlo, igual que hace el ejemplo)
  *
 **/
VSocket * SSLSocket::AcceptConnection() {
   int id;
   SSLSocket * peer;

   id = this->WaitForConnection();       // accept() de Unix, igual que en Socket::AcceptConnection

   peer = new SSLSocket( id, this->IPv6 );  // propagamos la familia (IPv4/IPv6) del socket que escucha

   return peer;
}


/**
  *  CopyContext
  *     Comparte el SSL_CTX* (contexto, con el certificado ya cargado) de
  *     "original" -osea del socket servidor que esta escuchando- con ESTE
  *     objeto (una conexion recien aceptada). Crea un SSL* (BIO) nuevo y
  *     propio para esta conexion, pero apuntando al MISMO contexto.
  *
  *
  *  @param     SSLSocket * original: el socket servidor (el que hizo Bind + MarkPassive)
  *
 **/
void SSLSocket::CopyContext( SSLSocket * original ) {

   // como CopyContext es un metodo de la MISMA clase SSLSocket, si puede
   // leer los miembros privados de "original" aunque sean otro objeto
   // (en C++ el control de acceso es por clase, no por instancia)
   this->Context = original->Context;
   this->ownsContext = false;   // dejamos explicito que este objeto NO es el dueno, solo lo esta usando prestado

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );
   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::CopyContext(): SSL_new failed" );
   }

   SSL_set_fd( ssl, this->sockId );   // atamos el descriptor de ESTA conexion (la nueva, no la del servidor) al SSL nuevo

   this->BIO = reinterpret_cast<void *>( ssl );

}


/**
  *  Accept
  *     hace el handshake SSL/TLS del lado servidor (SSL_accept), sobre
  *     una conexion que YA fue aceptada a nivel TCP (AcceptConnection())
  *     y ya tiene su SSL* armado (CopyContext()).
  *
  *  No confundir con AcceptConnection(): esta no espera clientes nuevos
  *  ni hace accept() de Unix, solo negocia el handshake SSL sobre una
  *  conexion que ya existe.
  *
  *  @returns   this, para poder encadenar llamadas si se quiere
  *     (ej: server->Accept()->ShowCerts();)
  *
 **/
SSLSocket * SSLSocket::Accept() {

   SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

   int st = SSL_accept( ssl );      // negocia el handshake TLS del lado servidor
   if ( st != 1 ) {
      int err = SSL_get_error( ssl, st );
      fprintf( stderr, "SSL_accept failed with error code %d\n", err );
      throw std::runtime_error( "SSLSocket::Accept(): SSL_accept failed" );
   }

   return this;

}