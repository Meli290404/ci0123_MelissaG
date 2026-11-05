#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "Mancomunado.h"

std::string armarMancomunado( int tipo, const std::vector<std::string> & campos ) {
    char codigo[ 8 ];
    snprintf( codigo, sizeof( codigo ), "%02d", tipo );

    std::string linea = codigo;
    for ( const auto & campo : campos ) {
        linea += "/" + campo;
    }
    return linea;
}

MensajeMancomunado parsearMancomunado( const std::string & linea ) {
    std::istringstream iss( linea );
    std::string token;

    // el tipo va primero para poder rechazar rapido un mensaje invalido
    if ( !std::getline( iss, token, '/' ) || token.size() != 2 ||
         !isdigit( (unsigned char) token[ 0 ] ) || !isdigit( (unsigned char) token[ 1 ] ) ) {
        throw std::invalid_argument( "parsearMancomunado: el tipo no son dos digitos" );
    }

    MensajeMancomunado msg;
    msg.tipo = std::stoi( token );
    while ( std::getline( iss, token, '/' ) ) {
        msg.campos.push_back( token );
    }
    return msg;
}
