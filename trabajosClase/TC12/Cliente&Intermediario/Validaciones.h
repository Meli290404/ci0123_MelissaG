#ifndef VALIDACIONES_H
#define VALIDACIONES_H

#include <string>

// validaciones campo por campo hechas a mano, sin <regex> en tiempo de
// ejecucion; las expresiones del documento de protocolo son la referencia
// de diseno, no lo que corre en el programa
bool validarOrigenDestino(const std::string& s);    // (CLI|INT)_NN o SERV
bool validarTipoMensaje(const std::string& s);       // 2 digitos
bool validarCategoria(const std::string& s);         // 3-20 letras
bool validarProducto(const std::string& s);          // 1-50, letras/digitos/guion
bool validarPrecio(const std::string& s);            // NNNNNN(.NN)?
bool validarStock(const std::string& s);             // 1-5 digitos
bool validarDescripcion(const std::string& s);       // 0-100 chars
bool validarCount(const std::string& s);             // 1-3 digitos
bool validarTamanoMensaje(const std::string& mensajeCompleto);

#endif