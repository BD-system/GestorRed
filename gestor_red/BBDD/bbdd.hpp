#pragma once

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <memory>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include "../equipos_red/EquiposRed.hpp"  // Ruta según tu proyecto

class BBDD {
private:
    std::unique_ptr<sql::Connection> conn;
    std::string host;
    std::string user;
    std::string password;
    std::string database;

public:
    BBDD(const std::string& host,
         const std::string& user,
         const std::string& password,
         const std::string& database);

    void conectar();
    void ejecutar(const std::string& query);
    std::unique_ptr<sql::ResultSet> ejecutarConsulta(const std::string& query);
    std::unique_ptr<sql::PreparedStatement> preparar(const std::string& query);
    sql::Connection* getConexion();
    ~BBDD();
};

// === Consultas SQL embebidas ===
extern const char* SQL_INIT_SCHEMA;
extern const char* SQL_COMPROBAR_RED;
extern const char* SQL_INSERTAR_RED;
extern const char* SQL_COMPROBAR_TABLA;
extern const char* SQL_CREAR_TABLA_RED;
extern const char* SQL_COMPROBAR_IP;
extern const char* SQL_INSERTAR_IP;

// === Lógica de administración ===
void bbdd_administracion(BBDD& db,
                         const std::string& ip_red,
                         const std::string& interfaz,
                         const std::string& mascara,
                         const std::string& broadcast,
                         const std::vector<EquiposRed>& dispositivos);

std::string ipRedToNombreTabla(const std::string& ip_red);
