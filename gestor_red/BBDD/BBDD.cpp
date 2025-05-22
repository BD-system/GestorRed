// /gestor_red/BBDD/BBDD.cpp
// BBDD.cpp - Implementación de la clase BBDD para gestionar la conexión con una base de datos MySQL.
// Usa el conector oficial de MySQL para C++ (MySQL Connector/C++).

#include "BBDD.hpp"
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <mysql_driver.h>
#include <stdexcept>
#include <iostream> 

// Constructor de la clase BBDD.
// Inicializa los parámetros de conexión y establece conexión con la base de datos.

BBDD::BBDD(const std::string& host,
           int port,
           const std::string& user,
           const std::string& password,
           const std::string& dbname)
    : host_(host), port_(port), user_(user), password_(password), dbname_(dbname), conn_(nullptr)
{
     // Obtención del driver MySQL (singleton).
    driver_ = sql::mysql::get_mysql_driver_instance();
    conectar();
}

// Destructor de la clase.
// Libera la conexión si está activa.
BBDD::~BBDD() {
    desconectar();
}

// Método privado que establece conexión con la base de datos.
// Si ya existe una conexión válida, no hace nada.+
void BBDD::conectar() {
    if (conn_ && conn_->isValid()) return;
    try {
        // Construcción del URI de conexión en formato "tcp://host:puerto".
        std::string uri = "tcp://" + host_ + ":" + std::to_string(port_);

        // Se establece la conexión con el servidor MySQL.
        conn_.reset(driver_->connect(uri, user_, password_));
        
         // Selección del esquema (base de datos).
        conn_->setSchema(dbname_);

    } catch (const sql::SQLException& e) {
        std::cerr << "[BBDD][ERROR] Error conectando a la base de datos: " << e.what() << std::endl;
        throw std::runtime_error("No se pudo conectar a la base de datos.");
    }
}

// Método privado que cierra la conexión si está abierta.
void BBDD::desconectar() {
    if (conn_) {
        conn_->close();
        conn_.reset();
    }
}

// Devuelve un puntero a la conexión activa.
// Si la conexión no es válida, intenta reconectar antes de devolverla.
sql::Connection* BBDD::getConexion() {
    if (!conn_ || !conn_->isValid())
        conectar();
    return conn_.get();
}

// Método que permite verificar si existe una conexión activa y válida.
bool BBDD::estaConectado() const {
    return conn_ && conn_->isValid();
}