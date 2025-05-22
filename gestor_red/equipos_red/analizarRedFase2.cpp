// /gestor_red/equipos_red/analizarRedFase2.cpp

#include <vector>  
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

// Esta es la segunda fase del análisis de red.
// Aquí amplío la información de cada equipo detectado en la fase anterior,
// realizando un escaneo completo de puertos y detección de sistema operativo.
std::vector<EquiposRed> EquiposRed::analizarRedFase2(const std::vector<EquiposRed>& dispositivos, const std::string& interfaz) {
    std::vector<EquiposRed> resultados;

    // Primero filtro solo los dispositivos que tienen IP válida.
    std::vector<EquiposRed> dispositivosValidos;
    for (const auto& d : dispositivos) {
        if (!d.getIP().empty()) {
            dispositivosValidos.push_back(d);
        }
    }

    size_t total = dispositivosValidos.size();
    size_t count = 0;

    for (const auto& dispositivo : dispositivosValidos) {
        ++count;

        // Imprimo una barra de progreso en consola con el porcentaje de avance.
        int percentage = static_cast<int>((count * 100.0) / total);
        int barWidth = 50;
        int posBar = (percentage * barWidth) / 100;
        std::ostringstream progressBar;
        progressBar << "[";
        for (int i = 0; i < posBar; ++i) progressBar << "#";
        for (int i = posBar; i < barWidth; ++i) progressBar << "-";
        progressBar << "] " << percentage << "%";

        // Mostrar progreso en dos líneas
        std::cout << "\r[DEBUG][Fase2] Analizando IP: " << dispositivo.getIP()
                  << " (" << count << "/" << total << ")\n"
                  << "[DEBUG][Fase2] " << progressBar.str() << std::flush;
        if (count < total) std::cout << "\033[F"; // Mover cursor una línea arriba

        // Determinar el estado:
        // Si el método es "Nmap-forzado" se asigna "vigilancia", de lo contrario se conserva el estado que ya tenía.
        std::string estado = (dispositivo.getMetodo() == "Nmap-forzado") ? "vigilancia" : dispositivo.getEstado();

        EquiposRed dispositivoActual(dispositivo.getIP(), estado);
        dispositivoActual.setMetodo(dispositivo.getMetodo());
        dispositivoActual.setMAC(dispositivo.getMAC());
        dispositivoActual.setVendor(dispositivo.getVendor());
        dispositivoActual.setInterfaz(dispositivo.getInterfaz());

        // Construyo el comando Nmap con escaneo SYN, detección de versión, sistema operativo y todos los puertos.
        std::ostringstream comando;
        comando << "nmap -Pn -sS -sV -O -n --max-retries 2 --host-timeout 20s -p- -e "
                << interfaz << " " << dispositivo.getIP();

        FILE* fp = popen(comando.str().c_str(), "r");
        if (!fp) {
            std::cerr << "[ERROR][Fase2] Fallo al ejecutar nmap para IP: " << dispositivo.getIP() << "\n";
            resultados.push_back(dispositivoActual);
            continue;
        }

        // Capturo toda la salida de Nmap en una única cadena.
        char buffer[1024];
        std::string output;
        while (fgets(buffer, sizeof(buffer), fp)) {
            output += buffer;
        }
        pclose(fp);

        // Busco puertos abiertos con su respectivo servicio detectado.
        std::regex puertoRegex("(\\d+)/tcp\\s+open\\s+([^\\n]+)");
        std::smatch match;
        std::string::const_iterator searchStart(output.cbegin());
        while (std::regex_search(searchStart, output.cend(), match, puertoRegex)) {
            int puerto = std::stoi(match[1]);
            std::string servicio = match[2];
            dispositivoActual.addPuerto(puerto);
            dispositivoActual.addBanner(puerto, servicio);
            searchStart = match.suffix().first;
        }

        // Intento extraer el sistema operativo detectado por Nmap.
        // Primero busco una línea con "OS details:"
        std::regex osDetailsRegex("OS details:\\s*(.*)");
        bool soDetectado = false;
        if (std::regex_search(output, match, osDetailsRegex)) {
            std::string osInfo = match[1];
            if (!osInfo.empty() && osInfo.find("and Service detection") == std::string::npos) {
                dispositivoActual.setSO(osInfo);
                soDetectado = true;
            }
        } else {
            std::regex osGuessRegex("Aggressive OS guesses:\\s*(.*)");
            if (std::regex_search(output, match, osGuessRegex)) {
                std::string osInfo = match[1];
                if (!osInfo.empty() && osInfo.find("and Service detection") == std::string::npos) {
                    dispositivoActual.setSO(osInfo);
                    soDetectado = true;
                }
            }
        }
        // Si no se detecta nada, marco el campo SO como "noDetectable".
        if (!soDetectado) {
            dispositivoActual.setSO("noDetectable");
        }

        // Extraigo el RTT si está disponible. Lo convierto de segundos a milisegundos.
        std::regex rttRegex("Host is up \\(([^)]+)s latency\\)");
        if (std::regex_search(output, match, rttRegex)) {
            double rtt = std::stod(match[1]);
            dispositivoActual.setRTT(static_cast<int>(rtt * 1000)); // en ms
        } else {
            dispositivoActual.setRTT(-1); // Si no aparece, marco como no disponible.
        }

        // Extraer TTL: buscar patrón "TTL: <número>"
        std::regex ttlRegex("TTL:\\s*(\\d+)");
        if (std::regex_search(output, match, ttlRegex)) {
            int ttl = std::stoi(match[1]);
            dispositivoActual.setTTL(ttl);
        } else {
            dispositivoActual.setTTL(-1);
        }

       // Por último, intento extraer la marca temporal si se proporciona.
        std::regex tsRegex("Timestamp:\\s*(\\S+)");
        if (std::regex_search(output, match, tsRegex)) {
            std::string ts = match[1];
            dispositivoActual.setTimestamp(ts);
        } else {
            dispositivoActual.setTimestamp(""); // Cadena vacía si no se encuentra
        }

        // Añado el dispositivo a la lista final de resultados.
        resultados.push_back(dispositivoActual);
    }

    std::cout << "\n";
    return resultados;
}
