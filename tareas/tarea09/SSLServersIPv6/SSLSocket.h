/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *   SSL Socket class interface
  *
  * (Fedora version)
  *
 **/

#ifndef SSLSocket_h
#define SSLSocket_h

#include "VSocket.h"


class SSLSocket : public VSocket {

public:
SSLSocket( bool IPv6 = false );           // Not possible to create with UDP, client constructor
SSLSocket( char *, char *, bool = false );      // For server connections
SSLSocket( int, bool = false );        // para conexiones aceptadas: descriptor + si es IPv6
~SSLSocket();
int Connect( const char *, int );
int Connect( const char *, const char * );
size_t Write( const char * );
size_t Write( const void *, size_t );
size_t Read( void *, size_t );
void ShowCerts();
const char * GetCipher();

VSocket * AcceptConnection();      // TCP accept "crudo" (hereda de VSocket, puro virtual, hay que implementarlo si o si)
void CopyContext( SSLSocket * original );  // comparte el SSL_CTX* de "original" (el socket que escucha) en vez de crear uno nuevo
SSLSocket * Accept();           // hace el handshake SSL/TLS (SSL_accept) sobre ESTE socket ya aceptado

private:
void InitSSL( bool = false );    // Defaults to create a client context, true if server context needed
void InitContext( bool );
void LoadCertificates( const char *, const char * );

// Instance variables      
void * Context;            // SSL context
void * BIO;          // SSL BIO (Basic Input/Output)
bool ownsContext;        // true solo para quien CREO el contexto (cliente o servidor original);
                    // false para las conexiones aceptadas que solo lo TOMAN PRESTADO via CopyContext.
                    // evita liberar (SSL_CTX_free) el mismo contexto mas de una vez.

};

#endif