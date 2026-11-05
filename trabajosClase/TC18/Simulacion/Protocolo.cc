#include <sstream>
#include <stdexcept>

#include "Protocolo.h"

std::string armarMensaje( const std::string & origen, const std::string & destino,
                           int tipo, const std::vector<std::string> & campos ) {
    std::ostringstream out;
    out << origen << "|" << destino << "/" << tipo;
    for ( const auto & campo : campos ) {
        out << "/" << campo;
    }
    return out.str();
}

MensajeProtocolo parsearMensaje( const std::string & linea ) {
    size_t pos_pipe = linea.find( '|' );
    if ( pos_pipe == std::string::npos ) {
        throw std::invalid_argument( "parsearMensaje: falta el separador '|'" );
    }

    MensajeProtocolo msg;
    msg.origen = linea.substr( 0, pos_pipe );

    std::string resto = linea.substr( pos_pipe + 1 );
    std::istringstream iss( resto );
    std::string token;

    if ( !std::getline( iss, token, '/' ) ) {
        throw std::invalid_argument( "parsearMensaje: falta el destino" );
    }
    msg.destino = token;

    if ( !std::getline( iss, token, '/' ) ) {
        throw std::invalid_argument( "parsearMensaje: falta el tipo de mensaje" );
    }
    try {
        msg.tipo = std::stoi( token );
    } catch ( const std::exception & ) {
        throw std::invalid_argument( "parsearMensaje: tipo de mensaje no es numerico" );
    }

    while ( std::getline( iss, token, '/' ) ) {
        msg.campos.push_back( token );
    }

    return msg;
}