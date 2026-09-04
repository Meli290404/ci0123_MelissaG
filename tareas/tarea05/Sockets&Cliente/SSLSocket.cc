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
}


/**
  *  Class constructor
  *
  *  @param     int id: socket descriptor
  *
 **/
SSLSocket::SSLSocket( int id ) {
   this->sockId = id;
   this->Context = nullptr;
   this->BIO = nullptr;
   this->InitSSL(false);
   SSL *ssl = (SSL *) this->BIO;
   SSL_set_fd(ssl, this->sockId);

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