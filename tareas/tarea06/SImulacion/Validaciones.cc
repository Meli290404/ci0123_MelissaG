#include <regex>
#include "Validaciones.h"

bool validarCategoria( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z]{3,20}$" );
    return std::regex_match( valor, patron );
}

bool validarProducto( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z0-9\\-]{1,50}$" );
    return std::regex_match( valor, patron );
}