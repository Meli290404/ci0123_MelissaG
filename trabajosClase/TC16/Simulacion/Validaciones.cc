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

bool validarCount( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,3}$" );
    return std::regex_match( valor, patron );
}

bool validarIdMensaje( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{4}$" );
    return std::regex_match( valor, patron );
}

bool validarIdIsla( const std::string & valor ) {
    static const std::regex patron( "^INT_[0-9]{2}$" );
    return std::regex_match( valor, patron );
}

bool validarIp( const std::string & valor ) {
    static const std::regex patron( "^([0-9]{1,3}\\.){3}[0-9]{1,3}$" );
    return std::regex_match( valor, patron );
}

bool validarPuerto( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,5}$" );
    return std::regex_match( valor, patron );
}

bool validarCategoriaMancomunado( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z0-9_\\-]{1,50}$" );
    return std::regex_match( valor, patron );
}

bool validarProductoMancomunado( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z0-9_\\-]{1,50}$" );
    return std::regex_match( valor, patron );
}

bool validarCantidadMancomunado( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,5}$" );
    return std::regex_match( valor, patron );
}
