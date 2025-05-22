// /gestor_red/BBDD/UtilsSQL.cpp

#include "UtilsSQL.hpp"
#include <sstream>
#include <algorithm>

// ---- Puertos CSV ----

// Convierte un vector de puertos (enteros) a una cadena separada por comas.
// Esto se usa para almacenar múltiples puertos en un solo campo de texto en SQL.
std::string UtilsSQL::puertosToString(const std::vector<int>& puertos) {
    std::ostringstream oss;
    for (size_t i = 0; i < puertos.size(); ++i) {
        if (i > 0) oss << ",";
        oss << puertos[i];
    }
    return oss.str();
}

// Realiza la operación inversa: convierte una cadena con puertos separados por comas
// a un vector de enteros. Útil para recuperar datos desde la base de datos.
std::vector<int> UtilsSQL::stringToPuertos(const std::string& str) {
    std::vector<int> puertos;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ',')) {
        if (!token.empty()) puertos.push_back(std::stoi(token));
    }
    return puertos;
}

// ---- Banners a "JSON manual" ----
// Serializa un mapa puerto -> banner en una cadena con formato similar a JSON.
// Esto permite guardar la estructura en un campo de texto en la base de datos.
std::string UtilsSQL::bannersToString(const std::map<int, std::string>& banners) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& par : banners) {
        if (!first) oss << ","; // Evita la coma inicial
        oss << "\"" << par.first << "\":\"" << par.second << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}


// Deserializa una cadena con formato similar a JSON a un mapa puerto -> banner.
// Nota: este parser es muy básico y no soporta comillas escapadas, se usa solo para fines internos controlados.
std::map<int, std::string> UtilsSQL::stringToBanners(const std::string& str) {
    // Solo para ejemplo, NO usar en producción si esperas cadenas con comillas/escapes
    std::map<int, std::string> banners;
    std::string s = str;
    // Elimino los caracteres de apertura y cierre del "JSON"
    s.erase(std::remove(s.begin(), s.end(), '{'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '}'), s.end());
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        size_t sep = token.find(':');
        if (sep != std::string::npos) {
            // Extraigo clave y valor, eliminando las comillas manualmente
            int puerto = std::stoi(token.substr(1, sep - 2));
            std::string banner = token.substr(sep + 2, token.size() - sep - 3);
            banners[puerto] = banner;
        }
    }
    return banners;
}

// ---- Vulnerabilidades a JSON minimalista ----

// Convierte una lista de vulnerabilidades (strings) en una cadena con formato de array JSON.
// Muy útil para almacenar vectores de strings en una sola columna de tipo TEXT o VARCHAR.
std::string UtilsSQL::vulnerabilidadesToString(const std::vector<std::string>& vulns) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vulns.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << vulns[i] << "\"";
    }
    oss << "]";
    return oss.str();
}

// Parsea una cadena con formato de array JSON a un vector de vulnerabilidades (strings).
// Este parser también es básico y no admite strings con comillas internas.
std::vector<std::string> UtilsSQL::stringToVulnerabilidades(const std::string& str) {
    std::vector<std::string> vulns;
    std::string s = str;

    // Elimino los corchetes del array
    s.erase(std::remove(s.begin(), s.end(), '['), s.end());
    s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
    std::istringstream iss(s);
    std::string token;
    while (std::getline(iss, token, ',')) {
        // Quito comillas de cada string
        token.erase(std::remove(token.begin(), token.end(), '\"'), token.end());
        if (!token.empty()) vulns.push_back(token);
    }
    return vulns;
}
