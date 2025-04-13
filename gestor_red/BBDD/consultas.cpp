#include "bbdd.hpp"

const char* SQL_COMPROBAR_RED = "SELECT COUNT(*) AS existe FROM RedesAnalizadas WHERE ip_red = ?";

const char* SQL_INSERTAR_RED = R"sql(
INSERT INTO RedesAnalizadas (ip_red, nombre_red, interfaz, mascara, broadcast)
VALUES (?, '', ?, ?, ?)
)sql";

const char* SQL_COMPROBAR_TABLA = "SHOW TABLES LIKE ?";

const char* SQL_CREAR_TABLA_RED = R"sql(
CREATE TABLE IF NOT EXISTS __TABLE_NAME__ (
    ip VARCHAR(45) PRIMARY KEY,
    mac VARCHAR(45),
    hostname VARCHAR(255),
    metodoDescubrimiento VARCHAR(255),
    sistemaOperativo VARCHAR(255),
    interfazDetectada VARCHAR(255),
    estado VARCHAR(255),
    ttl INT,
    rttMs INT,
    notas TEXT,
    vendor VARCHAR(255),
    timestamp VARCHAR(255),
    notasDetalladas TEXT,
    fecha_creacion TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;
)sql";
