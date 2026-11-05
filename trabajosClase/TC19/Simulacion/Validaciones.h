#ifndef VALIDACIONES_H
#define VALIDACIONES_H

#include <string>

// protocolo interno de la isla
bool validarCategoria( const std::string & valor ); // ^[a-zA-Z]{3,20}$
bool validarProducto( const std::string & valor );  // ^[a-zA-Z0-9\-]{1,50}$
bool validarCount( const std::string & valor );     // ^[0-9]{1,3}$

// protocolo mancomunado entre islas
bool validarIdMensaje( const std::string & valor );            // ^[0-9]{4}$
bool validarIdIsla( const std::string & valor );               // ^INT_[0-9]{2}$
bool validarIp( const std::string & valor );                   // ^([0-9]{1,3}\.){3}[0-9]{1,3}$
bool validarPuerto( const std::string & valor );               // ^[0-9]{1,5}$
bool validarCategoriaMancomunado( const std::string & valor ); // ^[a-zA-Z0-9_\-]{1,50}$
bool validarProductoMancomunado( const std::string & valor );  // ^[a-zA-Z0-9_\-]{1,50}$
bool validarCantidadMancomunado( const std::string & valor );  // ^[0-9]{1,5}$

#endif
