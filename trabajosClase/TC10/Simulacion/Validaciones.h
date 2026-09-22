#ifndef VALIDACIONES_H
#define VALIDACIONES_H

#include <string>

bool validarCategoria( const std::string & valor ); // ^[a-zA-Z]{3,20}$
bool validarProducto( const std::string & valor );  // ^[a-zA-Z0-9\-]{1,50}$
bool validarCount( const std::string & valor );     // ^[0-9]{1,3}$

#endif