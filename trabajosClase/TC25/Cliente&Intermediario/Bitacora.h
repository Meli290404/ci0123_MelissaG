#ifndef BITACORA_H
#define BITACORA_H

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <fstream>
#include <streambuf>

// bitacora de eventos de un programa (intermediario o servidor de productos).
// al crearla, std::cout y std::cerr pasan por ella: cada linea que el programa imprime se sigue
// viendo en la terminal y ademas queda en el archivo y en memoria con numero, fecha, hora y
// componente. las lineas de std::cerr quedan marcadas como ERROR
class Bitacora {
public:
    // las ultimas lineas que se guardan en memoria para mostrarlas en la pagina
    static const size_t MAX_EN_MEMORIA = 1000;

    Bitacora(const std::string& ruta, const std::string& componente);
    ~Bitacora();

    // la bitacora del programa; nullptr si todavia no se creo
    static Bitacora* actual();

    // agrega una linea al archivo y a la memoria (sin mostrarla en la terminal)
    void anotar(const std::string& texto, bool esError);

    // lineas con numero >= desde, como mucho las ultimas maximo. siguiente queda con el numero
    // de la proxima linea que se va a escribir
    std::vector<std::string> desde(long desde, long& siguiente, size_t maximo);

    // las lineas viajan en el mensaje 78, donde '/' y ';' son separadores: se escapan con %
    static std::string escapar(const std::string& linea);
    static std::string desescapar(const std::string& linea);

private:
    // streambuf que reemplaza al de std::cout o std::cerr: junta cada linea por hilo y,
    // al llegar el '\n', la muestra en la terminal y la anota
    class Desvio : public std::streambuf {
    public:
        Desvio(Bitacora& bitacora, std::streambuf* terminal, bool esError, int indice);
    protected:
        int overflow(int c) override;
    private:
        Bitacora& bitacora_;
        std::streambuf* terminal_;
        bool esError_;
        int indice_;   // 0 para cout y 1 para cerr, para separar el buffer de cada hilo
    };

    std::mutex mutex_;
    std::ofstream archivo_;
    std::string componente_;
    std::deque<std::string> lineas_;
    long siguiente_ = 1;

    std::streambuf* terminalSalida_;
    std::streambuf* terminalErrores_;
    Desvio salida_;
    Desvio errores_;

    void escribirTerminal(std::streambuf* terminal, const std::string& texto);

    static Bitacora* actual_;
};

#endif
