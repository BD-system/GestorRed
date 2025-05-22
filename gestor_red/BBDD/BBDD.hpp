// /gestor_red/BBDD/BBDD.hpp
// BBDD.hpp

#pragma once
#include <memory>
#include <string>

// Forward declaration para evitar incluir cabeceras pesadas de MySQL en otros módulos
namespace sql {
    class Connection;
    class Driver;
}

// Clase que encapsula la conexión con una base de datos MySQL
class BBDD {
private:
    // Parámetros de configuración de la conexión
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string dbname_;

    // Puntero único a la conexión y puntero al driver MySQL
    std::unique_ptr<sql::Connection> conn_;
    sql::Driver* driver_;


public:
    // Constructor con parámetros
    BBDD(const std::string& host,
         int port,
         const std::string& user,
         const std::string& password,
         const std::string& dbname);


    // Eliminar copia para evitar múltiples instancias con la misma conexión
    BBDD(const BBDD&) = delete;
    BBDD& operator=(const BBDD&) = delete;


    // Habilitar movimiento para permitir transferencia de propiedad
    BBDD(BBDD&&) noexcept = default;
    BBDD& operator=(BBDD&&) noexcept = default;

    // Destructor
    ~BBDD();

    // Abrir y cerrar conexión (puede reconectar)
    void conectar();

    // Cierra y libera la conexion
    void desconectar();

    // Devuelve puntero a la conexión activa (para operaciones de bajo nivel)
    sql::Connection* getConexion();

    // Verifica si la conexión está activa
    bool estaConectado() const;

    // Métodos utilitarios
    std::string getUsuario() const { return user_; }
    std::string getBaseDeDatos() const { return dbname_; }
};

