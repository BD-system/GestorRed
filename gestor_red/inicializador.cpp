// inicializador.cpp
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "equipos_red/EquiposRed.hpp"



struct InterfaceInfoRed {
    std::string nombre;
    std::string ip;
    std::string mac;
    std::string mascara;
    std::string ipRed;
    std::string broadcast;
    int totalEquipos;
};



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



bool guardarDatosEnBD(const std::vector<EquiposRed>& dispositivos) {
    std::cout << "[DEBUG] Llamada a guardarDatosEnBD().\n";
    for (const auto& d : dispositivos) d.imprimirResumen();
    return true;
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
    auto dispositivos = analizarRed(interfaces);

    if (!guardarDatosEnBD(dispositivos)) {
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

    std::cout << "[INFO] Inicialización completada. Se iniciaría la terminal interactiva ahora.\n";
}



int main() {
    inicializarSistema();
    return 0;
}
