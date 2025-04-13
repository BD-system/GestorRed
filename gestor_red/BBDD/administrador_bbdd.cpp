// administrador_bbdd.cpp

#include "bbdd.hpp"
#include "../equipos_red/EquiposRed.hpp"  // si es necesario para paso 2+
#include <algorithm>

void bbdd_administracion(BBDD& db,
                         const std::string& ip_red,
                         const std::string& interfaz,
                         const std::string& mascara,
                         const std::string& broadcast,
                         const std::vector<EquiposRed>& dispositivos) {
    try {
        // 1. Comprobar si la red ya existe
        std::unique_ptr<sql::PreparedStatement> comprobarRed = db.preparar(SQL_COMPROBAR_RED);
        comprobarRed->setString(1, ip_red);
        std::unique_ptr<sql::ResultSet> resultado = comprobarRed->executeQuery();

        bool redExiste = false;
        if (resultado->next()) {
            redExiste = resultado->getInt("existe") > 0;
        }

        if (!redExiste) {
            std::cout << "[INFO] Red nueva. Insertando en RedesAnalizadas...\n";
            std::unique_ptr<sql::PreparedStatement> insertarRed = db.preparar(SQL_INSERTAR_RED);
            insertarRed->setString(1, ip_red);
            insertarRed->setString(2, interfaz);
            insertarRed->setString(3, mascara);
            insertarRed->setString(4, broadcast);
            insertarRed->execute();
            std::cout << "[OK] Red insertada.\n";
        } else {
            std::cout << "[INFO] Red ya registrada en la base de datos.\n";
        }

        // 2. Generar nombre de tabla
        std::string nombreTabla = ipRedToNombreTabla(ip_red);

        // 3. Comprobar si la tabla existe
        std::unique_ptr<sql::PreparedStatement> comprobarTabla = db.preparar(SQL_COMPROBAR_TABLA);
        comprobarTabla->setString(1, nombreTabla);
        std::unique_ptr<sql::ResultSet> tablaRes = comprobarTabla->executeQuery();

        if (!tablaRes->next()) {
            std::cout << "[INFO] Tabla de red no existe. Será creada en el siguiente paso...\n";
            // Aquí iría el paso 2: crear tabla con SQL_CREAR_TABLA_RED
        } else {
            std::cout << "[INFO] Tabla de red ya existe: " << nombreTabla << "\n";
        }

    } catch (const sql::SQLException& e) {
        std::cerr << "[ERROR] Fallo durante administración de la red: " << e.what() << "\n";
    }
}
