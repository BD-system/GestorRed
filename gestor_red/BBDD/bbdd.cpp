// bbdd.cpp

#include "bbdd.hpp"

BBDD::BBDD(const std::string& host,
           const std::string& user,
           const std::string& password,
           const std::string& database)
    : host(host), user(user), password(password), database(database) {}

void BBDD::conectar() {
    try {
        sql::Driver* driver = get_driver_instance();
        conn.reset(driver->connect(host, user, password));
        conn->setSchema(database);
        std::cout << "[INFO] Conectado a la base de datos '" << database << "' en " << host << "\n";
    } catch (sql::SQLException& e) {
        std::cerr << "[ERROR] Fallo al conectar con la base de datos:\n"
                  << "Código: " << e.getErrorCode() << "\n"
                  << "SQLState: " << e.getSQLState() << "\n"
                  << "Mensaje: " << e.what() << "\n";
        throw;
    }
}

void BBDD::ejecutar(const std::string& query) {
    try {
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        stmt->execute(query);
    } catch (sql::SQLException& e) {
        std::cerr << "[ERROR] Error al ejecutar sentencia:\n" << query << "\n"
                  << "Código: " << e.getErrorCode() << "\n"
                  << "SQLState: " << e.getSQLState() << "\n"
                  << "Mensaje: " << e.what() << "\n";
        throw;
    }
}

std::unique_ptr<sql::ResultSet> BBDD::ejecutarConsulta(const std::string& query) {
    try {
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        return std::unique_ptr<sql::ResultSet>(stmt->executeQuery(query));
    } catch (sql::SQLException& e) {
        std::cerr << "[ERROR] Error al ejecutar consulta:\n" << query << "\n"
                  << "Código: " << e.getErrorCode() << "\n"
                  << "SQLState: " << e.getSQLState() << "\n"
                  << "Mensaje: " << e.what() << "\n";
        throw;
    }
}

std::unique_ptr<sql::PreparedStatement> BBDD::preparar(const std::string& query) {
    try {
        return std::unique_ptr<sql::PreparedStatement>(conn->prepareStatement(query));
    } catch (sql::SQLException& e) {
        std::cerr << "[ERROR] Error al preparar sentencia:\n" << query << "\n"
                  << "Código: " << e.getErrorCode() << "\n"
                  << "SQLState: " << e.getSQLState() << "\n"
                  << "Mensaje: " << e.what() << "\n";
        throw;
    }
}

sql::Connection* BBDD::getConexion() {
    return conn.get();
}

BBDD::~BBDD() {
    if (conn) {
        try {
            conn->close();
            std::cout << "[INFO] Conexión cerrada correctamente.\n";
        } catch (...) {
            std::cerr << "[WARN] Error al cerrar la conexión.\n";
        }
    }
}

std::string ipRedToNombreTabla(const std::string& ip_red) {
    std::string nombre = "red_" + ip_red;
    std::replace(nombre.begin(), nombre.end(), '.', '_');
    std::replace(nombre.begin(), nombre.end(), '/', '_');
    return nombre;
}


std::string crearQueryCrearTabla(const std::string& nombreTabla) {
    std::string query = SQL_CREAR_TABLA_RED;
    size_t pos = query.find("__TABLE_NAME__");
    if (pos != std::string::npos) {
        query.replace(pos, std::string("__TABLE_NAME__").length(), nombreTabla);
    }
    return query;
}

