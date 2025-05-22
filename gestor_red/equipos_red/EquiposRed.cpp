// /gestor_red/equipos_red/EquiposRed.cpp

#include "EquiposRed.hpp"
#include <iostream>
#include <vector>

// Función controladora que orquesta las fases de análisis de red
std::vector<EquiposRed> EquiposRed::analizarRed_gestor(const std::string& ip, 
                                                       const std::string& mask, 
                                                       const std::string& interfaz) {

    // Fase 1: Escaneo inicial para detectar dispositivos encendidos
    std::cout << "[DEBUG][Fase1] Iniciada\n";
    std::vector<EquiposRed> dispositivosFase1 = analizarRedFase1(ip, mask, interfaz);
    std::cout << "[DEBUG][Fase1] Completada: " << dispositivosFase1.size() << " dispositivos encontrados.\n";

    // Fase 2: Análisis detallado (puertos, detección de OS...)
    std::cout << "[DEBUG][Fase2] Iniciada...\n";
    std::vector<EquiposRed> dispositivosFase2 = analizarRedFase2(dispositivosFase1, interfaz);
    std::cout << "[DEBUG][Fase2] Completada: " << dispositivosFase2.size() << " dispositivos refinados.\n";

    // Resumen final de dispositivos activos: muestra IP, MAC (si existe) y el método de escaneo utilizado.
    std::cout << "\n[DEBUG][Finalizar] Resumen de dispositivos activos:\n";
    for (const auto& dispositivo : dispositivosFase2) {
        std::cout << "IP: " << dispositivo.getIP();
        if (!dispositivo.getMAC().empty()) {
            std::cout << " | MAC: " << dispositivo.getMAC();
        }
        std::cout << " | Método: " << dispositivo.getMetodo() << "\n";
    }
    
    return dispositivosFase2;
}
