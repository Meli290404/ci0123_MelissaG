#ifndef HTTPUTIL_H
#define HTTPUTIL_H

#include <string>

std::string urlEncode(const std::string& s);
std::string urlDecode(const std::string& s);
std::string buildGET(const std::string& path, const std::string& host);
int getCodigoEstado(const std::string& respuesta);
std::string extraerCuerpo(const std::string& respuesta);

#endif