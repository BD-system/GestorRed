// /gestor_red/equipos_red/InterfaceInfoRed.hpp

#pragma once
#include <string>

struct InterfaceInfoRed {
    std::string nombre;
    std::string ip;
    std::string mac;
    std::string mascara;
    std::string ipRed;
    std::string broadcast;
    int totalEquipos;
};
