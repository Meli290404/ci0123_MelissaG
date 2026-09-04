#include <iostream> 
#include <string>
#include <sstream> 
#include <vector> 
#include <iomanip>
#include <cstring> 

#include "VSocket.h" 
#include "Socket.h" 
#include "SSLSocket.h"

//server
static const char* HOST_SSL = "os.ecci.ucr.ac.cr";
static const char* HOST_PLANO_CASA = "163.178.104.62"; 
static const int PUERTO_PLANO = 80;

static const char* BASE_PATH = "/TicAmazon/list.php";
static const char* PARAM_CAT = "category"; 

//cat que espera el server
static const std::string CAT_EQUIPO = "Isla 4";

static const int MAXLINE = 4096; //tam buffer de lectura por cada Read()

//structs
struct Producto { 
    std::string intermediario;
    std::string bodega; 
    std::string categoria;
    std::string descripcion; 
    int cantidad = 0;
    double precio = 0.0; 
};

struct ItemFactura { 
    std::string nombre;
    int cant; 
    double precioCU; 
};
static std::vector<ItemFactura> shoppingCart;

//auxiliares

/**
 * urlEncode - codifica un string para meterlo en un query string de
 * una URL. No toca letras/numeros, convierte espacio a %20 y todo lo demas a su codigo hex %XX
 */
std::string urlEncode(const std::string& s) {
    std::ostringstream enc; //para armar el string encodeado
    for (unsigned char c : s) { //recorremos cada char del string og
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            enc << c; 
        } else if (c == ' ') {
            enc << "%20"; 
        } else { //cualquier otro char especial
            enc << '%' << std::uppercase << std::hex << std::setw(2)
                << std::setfill('0') << (int)c << std::nouppercase << std::dec; //codigo hex de 2 dig, con cero a la izquierda si hace falta
        }
    }
    return enc.str();
}



/**
 * buildGET - arma texto de una request HTTP GET, en el
 * formato que espera el servidor "HTTP/1.1" pegado a la ruta, y el header host
 * como el string literal 
 */
std::string buildGET(const std::string& path) {
    return "GET " + path + " HTTP/1.1\r\nhost: redes.ecci\r\nConnection: close\r\n\r\n"; //Connection: close hace que el servidor cierre al terminar
}


/**
 * getCodigoEstado - finds el codigo de estado HTTP en
 * la primera linea de la respuesta ("HTTP/1.1 200 OK..."), usando find/substr 
 */
int getCodigoEstado(const std::string& resp) {
    if (resp.rfind("HTTP/", 0) != 0) return -1; //if- no arranca con "HTTP/" no es una respuesta HTTP valida
    size_t espacio1 = resp.find(' ');
    if (espacio1 == std::string::npos) return -1; //no encontro espacio, weird
    size_t espacio2 = resp.find(' ', espacio1 + 1);
    if (espacio2 == std::string::npos) return -1; //no encontro el segundo espacio
    std::string codigoStr = resp.substr(espacio1 + 1, espacio2 - espacio1 - 1);
    try {
        return std::stoi(codigoStr); //turns a int
    } catch (...) {
        return -1; //if not valido, return -1
    }
}


/**
 * extraerCuerpo - separa el body de la respuesta HTTP,
 * corta after linea en blanco que separa .h del contenido
 */
std::string extraerCuerpo(const std::string& resp) {
    size_t pos = resp.find("\r\n\r\n"); //blankLine marca fin de headers
    if (pos == std::string::npos) return resp;  //if server no manda headers, returns todo el string como body
    return resp.substr(pos + 4); //todo despues de blankLine = cuerpo
}


/**
 * replace - replaces apariciones de buscar' por
 * 'reemplazo' dentro de 's', en el mismo string 
 */
void replace(std::string& s, const std::string& buscar, const std::string& reemplazo) {
    size_t pos = 0; //posicion desde donde seguimos buscando
    while ((pos = s.find(buscar, pos)) != std::string::npos) { //mientras sigamos encontrando 'buscar'
        s.replace(pos, buscar.size(), reemplazo); //reemplazamos esa aparicion
        pos += reemplazo.size(); //avanzamos despues del reemplazo, para no volver a matchear ahi
    }
}


/**
 * cleanEtiquetas - deletes todo lo que este entre < >, recorre string char por char
 */
std::string cleanEtiquetas(const std::string& html) {
    std::string resultado; //texto sin tags
    resultado.reserve(html.size());
    bool dentroDeTag = false; //flag: entre un '< >'?
    for (char c : html) { //recorrer char por char
        if (c == '<') { dentroDeTag = true; resultado += '\n'; continue; } //etiqueta cambiada por salto linea
        if (c == '>') { dentroDeTag = false; continue; } //final etiqueta
        if (!dentroDeTag) resultado += c; //add tag si no estamos dentro de un tag
    }
    return resultado; 
}

/**
 * cleanHTML - limpia HTML: quita etiquetas, decodifica
 * (&nbsp; &amp; etc.) y colapsa espacios/lineas vacias
 */
std::string cleanHTML(const std::string& html) {
    std::string sinTags = cleanEtiquetas(html); //quita etiquetas HTML

    replace(sinTags, "&nbsp;", " ");
    replace(sinTags, "&amp;", "&"); 
    replace(sinTags, "&lt;", "<"); 
    replace(sinTags, "&gt;", ">"); 
    replace(sinTags, "&quot;", "\""); 
    replace(sinTags, "&#39;", "'");

    std::istringstream iss(sinTags); //reads linea por linea
    std::ostringstream oss; //arma resultado
    std::string linea;
    while (std::getline(iss, linea)) { //recorre lineas
        size_t ini = linea.find_first_not_of(" \t\r");
        size_t fin = linea.find_last_not_of(" \t\r"); 
        if (ini == std::string::npos) continue; 
        oss << linea.substr(ini, fin - ini + 1) << "\n"; //add linea trim
    }
    return oss.str();
}


/**
 * esNum - checks si string son digitos
 */
bool esNum(const std::string& s) {
    if (s.empty()) return false; //string empty no es numero
    for (char c : s) { //revisa cada char
        if (!isdigit((unsigned char)c)) return false; //si char no es digito, no es numero
    }
    return true;
}

/**
 * parseProd - extrae lista de producto del texto ya limpio de
 * HTML, se usa "Bodega-" como ancla para cada fila y se valida que
 * cantidad/precio sean numericos antes de aceptar la fila, si no,
 * se descarta esa fila en vez de mezclar campos invalidos
 */
std::vector<Producto> parseProd(const std::string& textoLimpio) {
    std::vector<std::string> lineas; //guarda cada linea no empty
    std::istringstream iss(textoLimpio);
    std::string linea;
    while (std::getline(iss, linea)) { //separa texto en lineas
        if (!linea.empty()) lineas.push_back(linea); //guarda las que no esten vacias
    }

    std::vector<Producto> productos; //result final
    for (size_t i = 0; i < lineas.size(); ++i) { //recorre todas las lineas
        if (lineas[i].rfind("Bodega-", 0) != 0) continue;  //si no empieza con "Bodega-", no es inicio de fila

        if (i + 4 >= lineas.size()) continue;  //no hay suficientes lineas despues para completar la fila
        const std::string& categoria = lineas[i + 1];
        const std::string& descripcion = lineas[i + 2]; 
        const std::string& cantidadStr = lineas[i + 3];
        const std::string& precioStr = lineas[i + 4];

        if (!esNum(cantidadStr) || !esNum(precioStr)) {
            //fila corrupta tokens sueltos del servidor
            continue; //si cantidad o precio no son numeros validos, salta
        }

        Producto p; //arma producto
        p.bodega = lineas[i]; //linea ancla
        p.categoria = categoria;
        p.descripcion = descripcion;
        p.cantidad = std::stoi(cantidadStr); //texto a int
        p.precio = std::stod(precioStr); //texto a double

        //intermediario esta en la linea "IntN" mas cercana hacia atras
        for (int j = (int)i - 1; j >= 0; --j) { //busca hacia atras desde la fila act
            if (lineas[j].rfind("Int", 0) == 0) { //primera linea que empiece con int es el intermediario
                p.intermediario = lineas[j];
                break;
            }
        }

        productos.push_back(p); //agrega producto ya armado a la lista
        i += 4;
    }
    return productos;
}


/**
 * printProd - prints en pantalla lista de producto parseados
 */
void printProd(const std::vector<Producto>& productos) {
    for (auto& p : productos) { //recorremos cada producto
        std::cout << "  - " << p.descripcion
                  << " | categoria: " << p.categoria
                  << " | bodega: " << p.bodega << " (" << p.intermediario << ")"
                  << " | cantidad: " << p.cantidad
                  << " | precio: " << p.precio << "\n"; //un renglon con todos los campos del producto
    }
}

//capa red
struct respuestaHTTP { //resultado de intentar hablar con el servidor
    bool exito = false; 
    bool usoSSL = false; 
    int codigo = -1; //-1 si no se pudo interpretar
    std::string cuerpoLimpio;
};

/**
 * read - reads lo disponible del socket, acumulando en un string.
 */
std::string read(VSocket* socket) {
    char buffer[MAXLINE]; //buffer temp para cada Read()
    std::string acumulado; //junta lo leido
    try {
        int leidos; //cuantos bytes trajo el ultimo Read()
        while ((leidos = socket->Read(buffer, MAXLINE - 1)) > 0) { //mientras siga llegando datos
            acumulado.append(buffer, leidos); //agregar al acumulado
        }
    } catch (const std::exception&) {
        //lo que ya se acumulo hasta aqui es lo que hay
    }
    return acumulado;
}

/**
 * getHTTP - GET al servidor de bodega. Intenta primero SSL y si falla, cae al
 * fallback sin SSL (IP publica, puerto 80)
 */
respuestaHTTP getHTTP(const std::string& path) {
    respuestaHTTP resultado; //resultado que se va a devolver

    //intento 1: SSL sobre IPv4
    {
        VSocket* client = nullptr; //puntero base
        try {
            client = new SSLSocket(); //IPv4 
            if (client->Connect(HOST_SSL, "https") == 0) {
                std::string peticion = buildGET(path); //arma texto del GET
                client->Write(peticion.c_str()); //mandado por socket SSL
                std::string respuesta = read(client);
                delete client; 

                resultado.exito = true; //request SSL works
                resultado.usoSSL = true; //marca que fue por SSL
                resultado.codigo = getCodigoEstado(respuesta); //extrae el codigo HTTP
                resultado.cuerpoLimpio = cleanHTML(extraerCuerpo(respuesta)); //body limpio de HTML
                return resultado;
            }
            delete client; //Connect fallo
        } catch (const std::exception& e) {
            std::cerr << "  (SSL fallo: " << e.what() << ", cayendo a HTTP plano)\n";
            delete client;
        }
    }

    //intento 2: sin SSL
    {
        VSocket* client = nullptr; //puntero base, apunta a un Socket plano
        try {
            client = new Socket('s');  //IPv4 
            if (client->Connect(HOST_PLANO_CASA, PUERTO_PLANO) != 0) { //!=0 = Connect fallo
                throw std::runtime_error("Connect (plano) fallo");
            }
            std::string peticion = buildGET(path); 
            client->Write(peticion.c_str()); //manda por socket plano
            std::string respuesta = read(client);
            delete client;

            resultado.exito = true; //la conexion plana funciono
            resultado.usoSSL = false; //marcamos que NO fue por SSL
            resultado.codigo = getCodigoEstado(respuesta); //codigo HTTP de la respuesta
            resultado.cuerpoLimpio = cleanHTML(extraerCuerpo(respuesta)); //body limpio
            return resultado; 

        } catch (const std::exception& e) {
            std::cerr << "  (conexion plana tambien fallo: " << e.what() << ")\n"; //ni SSL ni plano funcionaron
            delete client;
        }
    }

    resultado.exito = false; //dos intentos fallaron
    return resultado;
}

//casos prueba

/**
 * pruebaListCat - consulta al servidor por una categoria, prints resultados
 */
void pruebaListCat(const std::string& categoria, bool addCarrito) {
    std::cout << "\n[Cliente -> Servidor] RE_CAT " << categoria << "\n"; //request

    std::string path = std::string(BASE_PATH) + "?" + PARAM_CAT + "=" +
                        urlEncode(categoria); //arma ruta con el query param ya codificado
    respuestaHTTP r = getHTTP(path); //request SSL o fallback

    if (!r.exito) {
        std::cout << "[Servidor -> Cliente] ERR_COMM Servidor RE_CAT\n"; //no se pudo ni conectar
        return;
    }
    std::cout << "  (conexion via " << (r.usoSSL ? "SSL/HTTPS" : "HTTP plano")
              << ", codigo HTTP " << r.codigo << ")\n"; //mostrar como se conecto y el codigo HTTP que devolvio el server

    if (r.codigo != 200) {
        std::cout << "[Servidor -> Cliente] ERR_COMM_HTTP RE_CAT codigo=" << r.codigo << "\n"; //distinto de 200 = algo mal
        return;
    }

    std::vector<Producto> productos = parseProd(r.cuerpoLimpio); //parsea tabla de productos
    if (productos.empty()) {
        std::cout << "[Servidor -> Cliente] PROD_LIST_EMPT \"No hay productos "
                     "disponibles en la categoria '" << categoria << "'\"\n"; //not found ninguna fila valida
        return;
    }

    //si no se filtra por categoria - todas las categorias muestra preview
    bool listaCompleta = categoria.empty(); //true si el usuario pidio todas las categorias
    size_t limite = (listaCompleta && productos.size() > 5) ? 5 : productos.size(); //si es listado completo y hay muchos solo muestra 5

    std::cout << "[Servidor -> Cliente] PROD_LIST " << productos.size() << " items"; //cuantos productos found en total
    if (limite < productos.size()) std::cout << " (mostrando los primeros " << limite << ")"; 
    std::cout << "\n";
    std::vector<Producto> aMostrar(productos.begin(), productos.begin() + limite); 
    printProd(aMostrar);

    if (addCarrito) { //si piden agregar al carrito
        for (auto& p : productos) { //recorre all productos encontrados
            shoppingCart.push_back({p.descripcion, 1, p.precio}); //entra al carrito con cantidad 1
        }
    }
}

/**
 * aMinusculas - convierte un string completo a minusculas
 */
std::string aMinusculas(const std::string& s) {
    std::string r = s; //copia del string original
    for (char& c : r) c = (char)tolower((unsigned char)c); //cada char a minuscula
    return r;
}

/**
 * pruebaFindProd - finds producto por nombre dentro del catalogo completo
 */
void pruebaFindProd(const std::string& nombreProducto) {
    std::cout << "\n[Cliente -> Servidor] RE_PROD \"" << nombreProducto
              << "\" (buscando en el catalogo completo)\n"; 

    respuestaHTTP r = getHTTP(std::string(BASE_PATH) + "?" + PARAM_CAT + "="); //pedir todo el catalogo = vacio
    if (!r.exito) {
        std::cout << "[Servidor -> Cliente] ERR_COMM Servidor RE_PROD\n"; //no se pudo conectar
        return;
    }
    if (r.codigo != 200) {
        std::cout << "[Servidor -> Cliente] ERR_COMM_HTTP RE_PROD codigo=" << r.codigo << "\n";
        return;
    }

    std::vector<Producto> todos = parseProd(r.cuerpoLimpio); //parsea catalogo
    std::string buscado = aMinusculas(nombreProducto); //normaliza el texto buscado a minusculas

    std::vector<Producto> encontrados; //los que hagan match
    for (auto& p : todos) { //recorrer catalogo
        if (aMinusculas(p.descripcion).find(buscado) != std::string::npos) { //coincidencia parcial
            encontrados.push_back(p); //agrega a encontrados
        }
    }

    if (encontrados.empty()) {
        std::cout << "[Servidor -> Cliente] PROD_NOT_AVAIL \"" << nombreProducto
                  << " no encontrado\"\n"; //ninguna coincidencia
        return;
    }
    std::cout << "[Servidor -> Cliente] PROD " << encontrados.size()
              << " coincidencia(s)\n"; //cuantas coincidencias hubo
    printProd(encontrados); 
}

/**
 * pruebaErrComm - fuerza error
 */
void pruebaErrComm() {
    const char* hostPruebaFalla = "127.0.0.1"; //loopback local, para error rapido y sin depender de la red
    std::cout << "\n[Cliente -> Servidor] RE_CAT Isla4 (error de red)\n";
    VSocket* client = nullptr;
    try {
        client = new Socket('s'); //socket TCP plano normal
        int r = client->Connect(hostPruebaFalla, 9);  //puerto discard
        if (r != 0) throw std::runtime_error("Connect fallo"); //si Connect no devuelve 0, fuerzacatch
        delete client;  //destructor cierra el socket
        std::cout << "[Servidor -> Cliente] (puerto de prueba si contesto)\n"; //
    } catch (const std::exception& e) {
        std::cout << "[Servidor -> Cliente] ERR_COMM Servidor RE_CAT (" << e.what() << ")\n"; //caso esperado: fallo la conexion
        delete client;
    }
}


/**
 * pruebaAgregar - finds producto en el catalogo  y lo
 * agrega al carrito con la cantidad indicada, prioriza coincidencia
 * exacta de nombre; si no hay, usa la primera coincidencia parcial
 */
void pruebaAdd(const std::string& args) {
    size_t espacio = args.find_last_of(' '); //la cantidad va al final - separada por espacio
    if (espacio == std::string::npos || espacio == 0) {
        std::cout << "[Cliente] Uso: AGREGAR <nombre_producto> <cantidad>\n";
        return;
    }
    std::string nombre = args.substr(0, espacio); //todo antes del ultimo espacio es el nombre
    std::string cantidadStr = args.substr(espacio + 1); //lo ultimo es la cantidad
 
    if (!esNum(cantidadStr)) {
        std::cout << "[Cliente] Cantidad invalida: '" << cantidadStr << "'\n";
        return;
    }
    int cantidad = std::stoi(cantidadStr);
    if (cantidad <= 0) {
        std::cout << "[Cliente] La cantidad debe ser mayor a 0\n";
        return;
    }
 
    std::cout << "\n[Cliente -> Servidor] RE_PROD \"" << nombre
              << "\" (para agregar " << cantidad << " al carrito)\n";
 
    respuestaHTTP r = getHTTP(std::string(BASE_PATH) + "?" + PARAM_CAT + "="); //pedir todo el catalogo
    if (!r.exito) {
        std::cout << "[Servidor -> Cliente] ERR_COMM Servidor RE_PROD\n";
        return;
    }
    if (r.codigo != 200) {
        std::cout << "[Servidor -> Cliente] ERR_COMM_HTTP RE_PROD codigo=" << r.codigo << "\n";
        return;
    }
 
    std::vector<Producto> todos = parseProd(r.cuerpoLimpio);
    std::string buscado = aMinusculas(nombre);
 
    //finds match exacto de nombre
    for (auto& p : todos) {
        if (aMinusculas(p.descripcion) == buscado) {
            shoppingCart.push_back({p.descripcion, cantidad, p.precio});
            std::cout << "[Servidor -> Cliente] PROD_ADDED " << cantidad << " x "
                      << p.descripcion << " @ " << p.precio << "\n";
            return;
        }
    }
    //if not match exacto, toma primera coincidencia parcial
    for (auto& p : todos) {
        if (aMinusculas(p.descripcion).find(buscado) != std::string::npos) {
            shoppingCart.push_back({p.descripcion, cantidad, p.precio});
            std::cout << "[Servidor -> Cliente] PROD_ADDED " << cantidad << " x "
                      << p.descripcion << " @ " << p.precio
                      << " (coincidencia parcial de \"" << nombre << "\")\n";
            return;
        }
    }
 
    std::cout << "[Servidor -> Cliente] PROD_NOT_AVAIL \"" << nombre
              << " no encontrado, no se agrego al carrito\"\n";
}


/**
 * pruebaFactura - prints factura con lo que haya en el carrito
 */
void pruebaFactura() {
    std::cout << "\n---Factura TicAmazon ---\n";
    std::cout << "Categoria: " << CAT_EQUIPO << "\n";
    if (shoppingCart.empty()) {
        std::cout << "(sin productos agregados)\n"; //carrito vacio
    } else {
        double total = 0.0; //total a pagar
        std::cout << std::left << std::setw(20) << "Producto" << std::setw(8)
                  << "Cant." << std::setw(10) << "P.Unit" << "Subtotal\n";
        for (auto& item : shoppingCart) { //recorre cada linea del carrito
            double subtotal = item.cant * item.precioCU; //subtotal de esa linea
            total += subtotal; //sumado al total general
            std::cout << std::left << std::setw(20) << item.nombre
                      << std::setw(8) << item.cant
                      << std::fixed << std::setprecision(2)
                      << std::setw(10) << item.precioCU
                      << subtotal << "\n";
        }
        std::cout << "------------------------------------------------------\n"; 
        std::cout << "TOTAL: " << std::fixed << std::setprecision(2) << total << "\n";
    }
}


int main() {
    std::cout << "---TicAmazon---" << std::endl;
    std::cout << "---Isla 4 en Funcionamiento---" << std::endl;
    std::cout << "Comandos: <categoria> | PRODUCTO <nombre> | AGREGAR <nombre> <cantidad> "
                 "| FACTURA | ERROR | salir\n";


    std::string linea; 
    while (std::cout << "\n> " && std::getline(std::cin, linea)) { 

        std::string comando = aMinusculas(linea); // solo para reconocer el comando, sin tocar 'linea'

        if (comando == "salir") break; 
        if (comando == "error") {
            pruebaErrComm();
            continue; 
  
        }
        if (comando.rfind("producto ", 0) == 0) { 
            pruebaFindProd(linea.substr(9)); 
            continue;
        }
        if (comando.rfind("agregar ", 0) == 0) {
            pruebaAdd(linea.substr(8));
            continue;
        }
        if (comando == "factura") {
            pruebaFactura();
            continue;
        }
        pruebaListCat(linea, /*addCarrito=*/false); 
    }

    std::cout << "\n---Fin de pruebas---" << std::endl; 
    return 0;
}