// /gestor_red/BBDD/GestorBBDD.cpp

#include "GestorBBDD.hpp"
#include "ConsultasSQL.hpp"
#include "UtilsSQL.hpp"
#include <cppconn/prepared_statement.h>
#include <stdexcept>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>


// Constructor de la clase GestorBBDD
// Inicializa el gestor con una instancia ya creada de BBDD
GestorBBDD::GestorBBDD(BBDD* bbdd) : bbdd_(bbdd) {}


// Inserta un registro de un equipo en la tabla 'equipos_red'
// Utiliza una sentencia preparada con parámetros para evitar inyecciones SQL
bool GestorBBDD::insertarEquipo(const EquiposRed& equipo, const std::string& etiqueta) {
    
    try {
        // Se obtiene una conexión válida desde el objeto BBDD
        sql::Connection* conn = bbdd_->getConexion();
    
        // Se prepara la sentencia SQL de inserción definida en ConsultasSQL
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(ConsultasSQL::INSERT_EQUIPO));
    
        // Asignación de parámetros en el orden definido por la consulta
        stmt->setString(1, etiqueta);
        stmt->setString(2, equipo.getIP());
        stmt->setString(3, equipo.getMAC());
        stmt->setString(4, equipo.getHostname());
        stmt->setString(5, equipo.getMetodo());
        stmt->setString(6, UtilsSQL::puertosToString(equipo.getPuertos()));
        stmt->setString(7, UtilsSQL::bannersToString(equipo.getBanners()));
        stmt->setString(8, equipo.getSO());
        stmt->setString(9, equipo.getInterfaz());
        stmt->setString(10, equipo.getEstado());
        stmt->setInt(11, equipo.getTTL());
        stmt->setInt(12, equipo.getRTT());
        stmt->setString(13, equipo.getNotas());
        stmt->setString(14, equipo.getVendor());
        stmt->setString(15, UtilsSQL::vulnerabilidadesToString(equipo.getVulnerabilidades()));
        stmt->setString(16, equipo.getNotasDetalladas());
        // La fecha se pone con NOW() en la query

        // Ejecución de la consulta
        stmt->execute();
        return true;
    } catch (const sql::SQLException& e) {
        throw std::runtime_error(std::string("Error al insertar equipo en BBDD: ") + e.what());
    }
}

// Inserta un registro de interfaz de red en la tabla 'interfaces'
// También usa sentencias preparadas para garantizar seguridad y eficiencia
bool GestorBBDD::insertarInterfaz(const InterfaceInfoRed& interfaz) {
    try {
        sql::Connection* conn = bbdd_->getConexion();
        std::unique_ptr<sql::PreparedStatement> stmt(
            conn->prepareStatement(ConsultasSQL::INSERT_INTERFAZ));
        
        stmt->setString(1, interfaz.nombre);
        stmt->setString(2, interfaz.ip);
        stmt->setString(3, interfaz.mascara);
        stmt->setString(4, interfaz.ipRed);
        stmt->setString(5, interfaz.broadcast);
        stmt->setString(6, interfaz.mac);

        stmt->execute();
        return true;
    } catch (const sql::SQLException& e) {
        throw std::runtime_error(std::string("Error al insertar interfaz en BBDD: ") + e.what());
    }
}
