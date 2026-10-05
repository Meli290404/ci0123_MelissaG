#include "HttpUtil.h"
#include <sstream>
#include <iomanip>
#include <cctype>
#include <cstdlib>

std::string urlEncode(const std::string& s) {
    std::ostringstream enc;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            enc << c;
        } else if (c == ' ') {
            enc << "%20";
        } else {
            enc << '%' << std::uppercase << std::hex << std::setw(2)
                << std::setfill('0') << (int)c << std::nouppercase << std::dec;
        }
    }
    return enc.str();
}

std::string urlDecode(const std::string& s) {
    std::string resultado;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            std::string hex = s.substr(i + 1, 2);
            resultado += (char)strtol(hex.c_str(), nullptr, 16);
            i += 2;
        } else if (s[i] == '+') {
            resultado += ' ';
        } else {
            resultado += s[i];
        }
    }
    return resultado;
}

std::string buildGET(const std::string& path, const std::string& host) {
    return "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
}

int getCodigoEstado(const std::string& resp) {
    if (resp.rfind("HTTP/", 0) != 0) return -1;
    size_t espacio1 = resp.find(' ');
    if (espacio1 == std::string::npos) return -1;
    size_t espacio2 = resp.find(' ', espacio1 + 1);
    if (espacio2 == std::string::npos) return -1;
    try {
        return std::stoi(resp.substr(espacio1 + 1, espacio2 - espacio1 - 1));
    } catch (...) {
        return -1;
    }
}

std::string extraerCuerpo(const std::string& resp) {
    size_t pos = resp.find("\r\n\r\n");
    if (pos == std::string::npos) return resp;
    return resp.substr(pos + 4);
}