#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "equipos_red/EquiposRed.hpp"
#include "equipos_red/InterfaceInfoRed.hpp"
#include "BBDD/BBDD.hpp"
#include "BBDD/GestorBBDD.hpp"

// --- DECLARACIÓN: función que lanza la terminal (la implementas en terminal.cpp) ---
void lanzarTerminal();

// --- Calcula la red y broadcast de una interfaz ---
void calcularRedYBroadcast(const std::string& ipStr, const std::string& maskStr, std::string& ipRedStr, std::string& broadcastStr) {
    struct in_addr ipAddr, maskAddr, networkAddr, broadcastAddr;
    if (inet_pton(AF_INET, ipStr.c_str(), &ipAddr) != 1 || inet_pton(AF_INET, maskStr.c_str(), &maskAddr) != 1) {
        ipRedStr = "";
        broadcastStr = "";
        return;
    }
    networkAddr.s_addr = ipAddr.s_addr & maskAddr.s_addr;
    broadcastAddr.s_addr = networkAddr.s_addr | ~(maskAddr.s_addr);
    char networkBuffer[INET_ADDRSTRLEN];
    char broadcastBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &networkAddr, networkBuffer, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &broadcastAddr, broadcastBuffer, INET_ADDRSTRLEN);
    ipRedStr = networkBuffer;
    broadcastStr = broadcastBuffer;
}

// --- Detecta las interfaces físicas y calcula sus datos ---
std::vector<InterfaceInfoRed> detectarInterfaces() {
    std::vector<InterfaceInfoRed> interfaces;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return interfaces;
    }
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK)) {
            InterfaceInfoRed info;
            info.nombre = ifa->ifa_name;
            char ipBuffer[INET_ADDRSTRLEN];
            struct sockaddr_in *sa_ip = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
            inet_ntop(AF_INET, &(sa_ip->sin_addr), ipBuffer, INET_ADDRSTRLEN);
            info.ip = ipBuffer;
            char maskBuffer[INET_ADDRSTRLEN];
            struct sockaddr_in *sa_mask = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask);
            inet_ntop(AF_INET, &(sa_mask->sin_addr), maskBuffer, INET_ADDRSTRLEN);
            info.mascara = maskBuffer;
            calcularRedYBroadcast(info.ip, info.mascara, info.ipRed, info.broadcast);
            int fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (fd != -1) {
                struct ifreq ifr;
                memset(&ifr, 0, sizeof(ifr));
                strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
                if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
                    unsigned char *mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                    char macBuffer[18];
                    snprintf(macBuffer, sizeof(macBuffer), "%02x:%02x:%02x:%02x:%02x:%02x",
                             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                    info.mac = macBuffer;
                }
                close(fd);
            }
            info.totalEquipos = 0;
            interfaces.push_back(info);
        }
    }
    freeifaddrs(ifaddr);
    return interfaces;
}

// --- Pide al usuario etiquetas para cada interfaz ---
std::map<std::string, std::string> pedirEtiquetasPorInterfaz(const std::vector<InterfaceInfoRed>& interfaces) {
    std::map<std::string, std::string> etiquetasPorInterfaz;
    for (const auto& iface : interfaces) {
        std::cout << "[ETIQUETA] Introduce una etiqueta para la interfaz " << iface.nombre
                  << " (" << iface.ip << "): ";
        std::string etiqueta;
        std::getline(std::cin, etiqueta);
        if (etiqueta.empty()) etiqueta = "default"; // Fallback
        etiquetasPorInterfaz[iface.nombre] = etiqueta;
    }
    return etiquetasPorInterfaz;
}

// --- Analiza la red usando la info de las interfaces ---
std::vector<EquiposRed> analizarRed(const std::vector<InterfaceInfoRed>& interfaces) {
    std::vector<EquiposRed> resultados;
    std::cout << "[DEBUG] Entrando en analizarRed()\n";
    std::cout << "[DEBUG] Número de interfaces detectadas: " << interfaces.size() << "\n";

    for (const auto& interfaz : interfaces) {
        std::cout << "[DEBUG] Interfaz " << interfaz.nombre << ":\n";
        std::cout << "         IP:        " << interfaz.ip << "\n";
        std::cout << "         Máscara:   " << interfaz.mascara << "\n";
        std::cout << "         IP Red:    " << interfaz.ipRed << "\n";
        std::cout << "         Broadcast: " << interfaz.broadcast << "\n";
        std::cout << "         MAC:       " << interfaz.mac << "\n";

        try {
            auto encontrados = EquiposRed::analizarRed_gestor(interfaz.ipRed, interfaz.broadcast, interfaz.nombre);
            std::cout << "[DEBUG] <- Dispositivos encontrados en esta interfaz: " << encontrados.size() << "\n";
            resultados.insert(resultados.end(), encontrados.begin(), encontrados.end());
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Excepción al analizar la interfaz " << interfaz.nombre << ": " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[ERROR] Excepción desconocida en interfaz " << interfaz.nombre << "\n";
        }
    }

    std::cout << "[DEBUG] Total de dispositivos en todas las interfaces: " << resultados.size() << "\n";
    return resultados;
}

// --- Guarda los equipos en la base de datos con su etiqueta ---
bool guardarDatosEnBD(const std::vector<EquiposRed>& dispositivos, GestorBBDD& gestor, const std::map<std::string, std::string>& etiquetasPorInterfaz) {
    std::cout << "[DEBUG] Llamada a guardarDatosEnBD().\n";
    bool todoOK = true;
    for (const auto& d : dispositivos) {
        d.imprimirResumen();
        try {
            std::string interfaz = d.getInterfaz();
            std::string etiqueta = "default";
            if (etiquetasPorInterfaz.count(interfaz)) {
                etiqueta = etiquetasPorInterfaz.at(interfaz);
            }
            if (!gestor.insertarEquipo(d, etiqueta)) {
                std::cerr << "[ERROR] No se pudo insertar el equipo en la BBDD (IP: " << d.getIP() << ")\n";
                todoOK = false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Excepción al insertar equipo en BBDD: " << e.what() << "\n";
            todoOK = false;
        }
    }
    return todoOK;
}

// --- Guarda las interfaces en la base de datos ---
bool guardarInterfacesEnBD(const std::vector<InterfaceInfoRed>& interfaces, GestorBBDD& gestor) {
    bool todoOK = true;
    for (const auto& iface : interfaces) {
        try {
            if (!gestor.insertarInterfaz(iface)) {
                std::cerr << "[ERROR] No se pudo insertar la interfaz en la BBDD (Nombre: " << iface.nombre << ")\n";
                todoOK = false;
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Excepción al insertar interfaz en BBDD: " << e.what() << "\n";
            todoOK = false;
        }
    }
    return todoOK;
}

bool sincronizarBD() {
    std::cout << "[DEBUG] Llamada a sincronizarBD().\n";
    return true;
}

bool inicializarModulos(const std::string& rutaModulos) {
    std::cout << "[DEBUG] Llamada a inicializarModulos() para la ruta: " << rutaModulos << "\n";
    return true;
}

void inicializarSistema() {
    std::cout << "==================================\n";
    std::cout << " Inicializador del Sistema (Modo Normal) \n";
    std::cout << "==================================\n";

    auto interfaces = detectarInterfaces();
    auto etiquetasPorInterfaz = pedirEtiquetasPorInterfaz(interfaces);
    auto dispositivos = analizarRed(interfaces);

    BBDD conexionBBDD("localhost", 3306, "gestor_red", "Ydnqnuedu1qrq1FMQUE1q34", "gestor_red");
    GestorBBDD gestor(&conexionBBDD);

    if (!guardarInterfacesEnBD(interfaces, gestor)) {
        std::cerr << "[ERROR] Fallo al guardar las interfaces en la base de datos interna.\n";
        std::exit(EXIT_FAILURE);
    }

    if (!guardarDatosEnBD(dispositivos, gestor, etiquetasPorInterfaz)) {
        std::cerr << "[ERROR] Fallo al guardar los datos en la base de datos interna.\n";
        std::exit(EXIT_FAILURE);
    }

    if (!sincronizarBD()) {
        std::cerr << "[ERROR] Fallo en la sincronización de la base de datos.\n";
        std::exit(EXIT_FAILURE);
    }

    if (!inicializarModulos("/opt/gestor_red/modules/")) {
        std::cerr << "[ERROR] Fallo en la inicialización de los módulos.\n";
        std::exit(EXIT_FAILURE);
    }

    std::cout << "[INFO] Inicialización completada. Se iniciará la terminal interactiva ahora.\n";
}

int main() {
    inicializarSistema();
    lanzarTerminal(); // <-- Pasa el control a la terminal modular
    return 0;
}
