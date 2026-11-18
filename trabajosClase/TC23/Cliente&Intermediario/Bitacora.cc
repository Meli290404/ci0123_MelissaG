#include "Bitacora.h"

#include <iostream>
#include <ctime>
#include <cstdio>

Bitacora* Bitacora::actual_ = nullptr;

Bitacora* Bitacora::actual() {
    return actual_;
}

Bitacora::Bitacora(const std::string& ruta, const std::string& componente)
    : archivo_(ruta, std::ios::app), componente_(componente),
      terminalSalida_(std::cout.rdbuf()), terminalErrores_(std::cerr.rdbuf()),
      salida_(*this, terminalSalida_, false, 0), errores_(*this, terminalErrores_, true, 1) {
    actual_ = this;
    anotar("INICIO bitacora en " + ruta, false);
    // desde aqui todo lo que se imprime con cout y cerr pasa por la bitacora
    std::cout.rdbuf(&salida_);
    std::cerr.rdbuf(&errores_);
}

Bitacora::~Bitacora() {
    std::cout.rdbuf(terminalSalida_);
    std::cerr.rdbuf(terminalErrores_);
    if (actual_ == this) actual_ = nullptr;
}

void Bitacora::anotar(const std::string& texto, bool esError) {
    char hora[32];
    time_t ahora = time(nullptr);
    strftime(hora, sizeof(hora), "%Y-%m-%d %H:%M:%S", localtime(&ahora));

    std::lock_guard<std::mutex> lock(mutex_);
    std::string linea = std::to_string(siguiente_++) + " " + hora + " [" + componente_ + "] " +
                        (esError ? "ERROR " : "") + texto;
    // flush en cada linea: si el programa se cae, lo anotado ya esta en el archivo
    archivo_ << linea << "\n";
    archivo_.flush();
    lineas_.push_back(linea);
    if (lineas_.size() > MAX_EN_MEMORIA) lineas_.pop_front();
}

void Bitacora::escribirTerminal(std::streambuf* terminal, const std::string& texto) {
    std::lock_guard<std::mutex> lock(mutex_);
    terminal->sputn(texto.data(), texto.size());
    terminal->pubsync();
}

std::vector<std::string> Bitacora::desde(long desde, long& siguiente, size_t maximo) {
    std::lock_guard<std::mutex> lock(mutex_);
    siguiente = siguiente_;

    // la primera linea en memoria es la numero siguiente_ - lineas_.size()
    long primera = siguiente_ - (long)lineas_.size();
    long inicio = std::max(desde, primera);
    if ((long)maximo < siguiente_ - inicio) inicio = siguiente_ - (long)maximo;

    std::vector<std::string> resultado;
    for (long n = inicio; n < siguiente_; n++) resultado.push_back(lineas_[n - primera]);
    return resultado;
}

std::string Bitacora::escapar(const std::string& linea) {
    std::string resultado;
    for (char c : linea) {
        if (c == '%') resultado += "%25";
        else if (c == '/') resultado += "%2F";
        else if (c == ';') resultado += "%3B";
        else if (c == '\n' || c == '\r') resultado += ' ';
        else resultado += c;
    }
    return resultado;
}

std::string Bitacora::desescapar(const std::string& linea) {
    std::string resultado;
    for (size_t i = 0; i < linea.size(); i++) {
        if (linea[i] == '%' && i + 2 < linea.size()) {
            std::string codigo = linea.substr(i + 1, 2);
            if (codigo == "25") { resultado += '%'; i += 2; continue; }
            if (codigo == "2F") { resultado += '/'; i += 2; continue; }
            if (codigo == "3B") { resultado += ';'; i += 2; continue; }
        }
        resultado += linea[i];
    }
    return resultado;
}

Bitacora::Desvio::Desvio(Bitacora& bitacora, std::streambuf* terminal, bool esError, int indice)
    : bitacora_(bitacora), terminal_(terminal), esError_(esError), indice_(indice) {}

// sin area de salida propia, cada caracter llega aqui. cada hilo junta su linea en su propio
// buffer, asi dos hilos que imprimen al mismo tiempo no mezclan sus lineas
int Bitacora::Desvio::overflow(int c) {
    thread_local std::string pendiente[2];
    if (c == traits_type::eof()) return traits_type::not_eof(c);

    std::string& linea = pendiente[indice_];
    if (c != '\n') {
        linea += (char)c;
        return c;
    }
    bitacora_.escribirTerminal(terminal_, linea + "\n");
    bitacora_.anotar(linea, esError_);
    linea.clear();
    return c;
}
