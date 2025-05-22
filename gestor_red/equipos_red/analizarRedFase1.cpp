// /gestor_red/equipos_red/analizarRedFase1.cpp

#include "equipos_red/EquiposRed.hpp"
#include <cstdio>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <algorithm>
#include <cctype>

// Elimina espacios y caracteres de control al inicio y al final de una cadena.
// Esto es útil para limpiar líneas obtenidas desde la salida de comandos como nmap.
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \n\r\t");
    if (start == std::string::npos)
        return "";  // La cadena está vacía o contiene solo espacios.
    size_t end = s.find_last_not_of(" \n\r\t");
    return s.substr(start, end - start + 1);
}


// Calcula el prefijo CIDR correspondiente al rango entre la IP de red y la de broadcast.
// Esto se usa para determinar el rango que se va a escanear con Nmap.
int calcularCIDR(const std::string& ipRed, const std::string& broadcast) {
    in_addr red, bcast;
    inet_pton(AF_INET, ipRed.c_str(), &red);
    inet_pton(AF_INET, broadcast.c_str(), &bcast);
    uint32_t n_red = ntohl(red.s_addr);
    uint32_t n_bcast = ntohl(bcast.s_addr);
    uint32_t diferencia = n_bcast - n_red + 1;

    int bits = 32;
    while (diferencia > 1) {
        diferencia >>= 1;
        --bits;
    }
    return bits;
}

// Verifica si una cadena es una dirección IPv4 válida.
// Se utiliza una expresión regular que cubre el rango válido de cada octeto.
bool ipValida(const std::string& ip) {
    if (ip.empty() || ip.size() > 32) return false;
    std::regex ipRegex("^((25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)\\.){3}(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)$");
    return std::regex_match(ip, ipRegex);
}

// Fase 1 del análisis de red: descubrimiento básico y detección forzada de dispositivos activos.
// Esta función realiza un escaneo de red mediante Nmap y devuelve una lista de equipos detectados.
std::vector<EquiposRed> EquiposRed::analizarRedFase1(const std::string& ipRed,
                                                     const std::string& broadcast,
                                                     const std::string& interfaz)
{
    std::vector<EquiposRed> encontrados;
    encontrados.reserve(256); // Reservo memoria anticipadamente para optimizar inserciones.
    std::set<std::string> ipsDetectadas; // Para evitar duplicados.
    int cidr = calcularCIDR(ipRed, broadcast); // Determinao el prefijo para el escaneo.

    // SCAN 1: Descubrimiento inicial con nmap (mensaje resumido)
    std::cout << "[DEBUG][Fase1] Buscar dispositivos encendidos en la red\n";
    {
        std::ostringstream comando;
        comando << "nmap -sn -n --min-parallelism 10 --max-retries 1 --host-timeout 500ms -e "
                << interfaz << " " << ipRed << "/" << cidr;
        FILE* fp = popen(comando.str().c_str(), "r");
        if (!fp) {
            std::cerr << "[ERROR] Fallo al ejecutar Nmap [Fase1]\n";
            return encontrados;
        }

        char buffer[512];
        std::string ipActual;
        while (fgets(buffer, sizeof(buffer), fp)) {
            std::string linea(buffer);
            // Detecto líneas que contienen IPs detectadas por Nmap.
            if (linea.find("Nmap scan report for") != std::string::npos) {
                size_t pos = linea.find_last_of(' ');
                if (pos != std::string::npos) {
                    ipActual = trim(linea.substr(pos + 1));
                    if (!ipValida(ipActual)) continue;
                    ipsDetectadas.insert(ipActual);
                    
                    // Asigno el estado "normal" porque fue detectado en un escaneo estándar.
                    EquiposRed equipo(ipActual, "normal");
                    equipo.setMetodo("Nmap-ping");
                    equipo.setInterfaz(interfaz);
                    encontrados.push_back(equipo);
                }
            }
            // También intento capturar la MAC y el fabricante si está disponible.
            else if (linea.find("MAC Address:") != std::string::npos && !ipActual.empty()) {
                std::regex macRegex("MAC Address: ([0-9A-Fa-f:]{17}) ?(.*)?");
                std::smatch match;
                if (std::regex_search(linea, match, macRegex)) {
                    std::string mac = match[1];
                    std::string vendor = match.size() > 2 ? match[2].str() : "";
                    for (auto& e : encontrados) {
                        if (e.getIP() == ipActual) {
                            e.setMAC(mac);
                            e.setVendor(vendor);
                            break;
                        }
                    }
                }
            }
        }
        pclose(fp);
        std::cout << "[DEBUG][Fase1] Completada: " << encontrados.size() << " dispositivos encontrados.\n";
    }

    // SEGUNDO ESCANEO: para las IPs que no respondieron antes, fuerzo un escaneo individual.
    std::cout << "[DEBUG][Scan2] Detección forzada de IPs no detectadas previamente\n";
    {
        in_addr red, bcast;
        inet_pton(AF_INET, ipRed.c_str(), &red);
        inet_pton(AF_INET, broadcast.c_str(), &bcast);
        uint32_t start = ntohl(red.s_addr);
        uint32_t end = ntohl(bcast.s_addr);
        uint32_t totalIPs = end - start - 1;  // Total de IPs a escanear
        uint32_t currentCount = 0;

        for (uint32_t ip = start + 1; ip < end; ++ip) {
            currentCount++;

            struct in_addr addr;
            addr.s_addr = htonl(ip);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr, ipStr, INET_ADDRSTRLEN);
            std::string ipCandidata = ipStr;
            
            // Construyo e imprimo una barra de progreso en la terminal.
            int percentage = static_cast<int>((currentCount * 100.0) / totalIPs);
            int barWidth = 50;
            int posBar = (percentage * barWidth) / 100;
            std::ostringstream progressBar;
            progressBar << "[";
            for (int i = 0; i < posBar; ++i)
                progressBar << "#";
            for (int i = posBar; i < barWidth; ++i)
                progressBar << "-";
            progressBar << "] " << percentage << "%";
            
            // Imprimir la línea con la IP actual
            std::cout << "\r[DEBUG][Scan2] Escaneando IP: " << ipCandidata 
                      << " (" << currentCount << "/" << totalIPs << ")\n";
            // Imprimir la barra de progreso en la línea siguiente
            std::cout << "[DEBUG][Scan2] " << progressBar.str() << std::flush;
            
            // Para actualizar dinámicamente, mover el cursor una línea hacia arriba (a la barra)
            if (ip < end - 1)
                std::cout << "\033[F"; // Muevo el cursor hacia arriba para sobrescribir la línea anterior.

            // Si la IP ya fue detectada o no es válida, salta el escaneo
            if (ipsDetectadas.count(ipCandidata)) continue;
            if (!ipValida(ipCandidata)) continue;

            std::ostringstream cmd;
            cmd << "nmap -sn -n --max-retries 1 --host-timeout 500ms -e "
                << interfaz << " " << ipCandidata;

            FILE* fp2 = popen(cmd.str().c_str(), "r");
            if (!fp2) {
                std::cerr << "[ERROR] No se pudo ejecutar nmap para IP: " << ipCandidata << "\n";
                continue;
            }
            bool encontrado = false;
            bool scanReportFound = false;
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), fp2)) {
                std::string linea(buffer);
                if (linea.find("Nmap scan report for") != std::string::npos) {
                    scanReportFound = true;
                }
                else if (linea.find("Host is up") != std::string::npos && scanReportFound) {
                    EquiposRed equipo(ipCandidata, "vigilancia");
                    equipo.setMetodo("Nmap-forzado");
                    equipo.setInterfaz(interfaz);
                    encontrados.push_back(equipo);
                    encontrado = true;
                    break;
                }
                else if (linea.find("MAC Address:") != std::string::npos) {
                    std::regex macRegex("MAC Address: ([0-9A-Fa-f:]{17}) ?(.*)?");
                    std::smatch match;
                    if (std::regex_search(linea, match, macRegex)) {
                        std::string mac = match[1];
                        std::string vendor = match.size() > 2 ? match[2].str() : "";
                        for (auto& e : encontrados) {
                            if (e.getIP() == ipCandidata) {
                                e.setMAC(mac);
                                e.setVendor(vendor);
                                break;
                            }
                        }
                    }
                }
            }
            pclose(fp2);
            // Si no se detectó \"Host is up\", se añade un registro con IP vacía (no se mostrará en el resumen)
            if (!encontrado) {
                EquiposRed equipo("", "vigilancia");
                equipo.setMetodo("Nmap-forzado");
                equipo.setInterfaz(interfaz);
                encontrados.push_back(equipo);
            }
        }
        // Salto de línea final después de la barra de progreso
        std::cout << "\n";
    }

    // Mostrar resumen de dispositivos activos (solo se muestran aquellos con IP asignada)
    std::cout << "\nResumen de dispositivos activos:\n";
    for (const auto& d : encontrados) {
        if (!d.getIP().empty()) {
            std::cout << "IP: " << d.getIP();
            if (!d.getMAC().empty())
                std::cout << " | MAC: " << d.getMAC();
            std::cout << " | Método: " << d.getMetodo() << "\n";
        }
    }
    return encontrados;
}
