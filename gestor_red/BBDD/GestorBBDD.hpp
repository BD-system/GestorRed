// /gestor_red/BBDD/GestorBBDD.hpp

#pragma once

#include "BBDD.hpp"
#include "../equipos_red/EquiposRed.hpp"
#include "../equipos_red/InterfaceInfoRed.hpp"
#include <string>

// Clase que gestiona operaciones de alto nivel sobre la BBDD
class GestorBBDD {
private:
    BBDD* bbdd_; // No gestiona la vida de la conexión aquí

public:
    // Constructor (recibe puntero o referencia a la conexión activa)
    explicit GestorBBDD(BBDD* bbdd);

    // Inserta un equipo en la tabla equipos_red
    // Devuelve true si la inserción fue correcta, false si no
    // Lanza excepción std::runtime_error si hay un error crítico de BBDD
    bool insertarInterfaz(const InterfaceInfoRed& interfaz);
    bool insertarEquipo(const EquiposRed& equipo, const std::string& etiqueta);
};
