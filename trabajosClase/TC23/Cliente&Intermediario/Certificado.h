#ifndef CERTIFICADO_H
#define CERTIFICADO_H

#include <iostream>
#include <cstdlib>
#include <sys/stat.h>

//certificado y llave de los programas que escuchan por TLS 
//se crea con make certificado
static const char* const ARCHIVO_CERTIFICADO = "certificados/ci0123.pem";

//sin certificado no se puede atender por TLS, se explica y se termina
//true si ya se creo el certificado con make certificado
inline bool hayCertificado() {
    struct stat info;
    return stat(ARCHIVO_CERTIFICADO, &info) == 0;
}

inline void exigirCertificado() {
    struct stat info;
    if (stat(ARCHIVO_CERTIFICADO, &info) == 0) return;

    std::cerr << "no se encontro " << ARCHIVO_CERTIFICADO << ". Se crea con:\n"
              << "  make certificado\n"
              << "que hace lo mismo que:\n"
              << "  mkdir -p certificados\n"
              << "  openssl req -x509 -newkey rsa:2048 -keyout certificados/key.pem -out certificados/cert.pem"
                 " -days 365 -nodes -subj \"/C=CR/O=UCR/CN=ci0123\"\n"
              << "  cat certificados/key.pem certificados/cert.pem > " << ARCHIVO_CERTIFICADO << "\n";
    std::exit(1);
}

#endif
