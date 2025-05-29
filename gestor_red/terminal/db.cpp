#include "db.hpp"
#include "style_terminal.hpp"
#include "../BBDD/BBDD.hpp" 
#include <cppconn/connection.h>
#include <iostream>
#include <map>
#include <vector>
#include <regex>
#include <sstream>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <iomanip>

static BBDD gBBDD("localhost", 3306, "gestor_red", "Ydnqnuedu1qrq1FMQUE1q34", "gestor_red");

static const std::map<std::string, std::string> db_aliases = {
    // 1.1
    { "equipos",
      "SELECT * FROM equipos_red;" },
    // 1.2
    { "equipos ultimo_scan",
      "SELECT * FROM equipos_red\n"
      " WHERE DATE_FORMAT(fecha, '%Y-%m-%d %H') = (\n"
      "     SELECT DATE_FORMAT(MAX(fecha), '%Y-%m-%d %H')\n"
      "     FROM equipos_red\n"
      " );" },
    // 1.3
    { "equipos ip=192.168.1.50",
      "SELECT * FROM equipos_red WHERE ip='192.168.1.50';" },
    // 1.4
    { "equipos mac=00:11:22:33:44:55",
      "SELECT * FROM equipos_red WHERE mac='00:11:22:33:44:55';" },
    // 1.5
    { "equipos hostname=host01",
      "SELECT * FROM equipos_red WHERE hostname='host01';" },

    // 2.1
    { "equipos etiqueta=servidor",
      "SELECT * FROM equipos_red WHERE etiqueta='servidor';" },
    // 2.2
    { "equipos estado=activo",
      "SELECT * FROM equipos_red WHERE estado='activo';" },
    // 2.3
    { "equipos estado=vigilancia",
      "SELECT * FROM equipos_red WHERE estado='vigilancia';" },
    // 2.4
    { "equipos so=Windows",
      "SELECT * FROM equipos_red WHERE sistema_operativo LIKE '%Windows%';" },
    // 2.5
    { "equipos metodo=Nmap",
      "SELECT * FROM equipos_red WHERE metodo_descubrimiento='Nmap';" },

    // 3.1
    { "equipos con_vulnerabilidades",
      "SELECT ip, mac, vulnerabilidades FROM equipos_red "
      "WHERE vulnerabilidades IS NOT NULL AND vulnerabilidades != '';" },
    // 3.2
    { "equipos puerto=22",
      "SELECT * FROM equipos_red WHERE puertos_abiertos LIKE '%22%';" },
    // 3.3
    { "equipos interfaz=eth0",
      "SELECT * FROM equipos_red WHERE interfaz_detectada='eth0';" },
    // 3.4
    { "equipos rtt_ms>100",
      "SELECT * FROM equipos_red WHERE rtt_ms > 100;" },
    // 3.5
    { "equipos con_notas",
      "SELECT * FROM equipos_red WHERE notas IS NOT NULL AND notas != '';" },

    // 4.1
    { "estados",
      "SELECT DISTINCT estado FROM equipos_red;" },
    // 4.2
    { "sistemas",
      "SELECT DISTINCT sistema_operativo FROM equipos_red;" },
    // 4.3
    { "resumen estados",
      "SELECT estado, COUNT(*) FROM equipos_red GROUP BY estado;" },
    // 4.4
    { "resumen so",
      "SELECT sistema_operativo, COUNT(*) FROM equipos_red GROUP BY sistema_operativo;" },
    // 4.5
    { "equipos fecha=2024-05-30",
      "SELECT * FROM equipos_red WHERE DATE(fecha) = '2024-05-30';" },

    // 5.1
    { "equipos ultimos7dias",
      "SELECT * FROM equipos_red WHERE fecha >= NOW() - INTERVAL 7 DAY;" },
    // 5.2
    { "metodos",
      "SELECT DISTINCT metodo_descubrimiento FROM equipos_red;" },
    // 5.3
    { "banners puerto=80",
      "SELECT ip, banners FROM equipos_red WHERE puertos_abiertos LIKE '%80%';" },
    // 5.4
    { "equipos vendor=Cisco",
      "SELECT * FROM equipos_red WHERE vendor LIKE '%Cisco%' ;" },
    // 5.5
    { "etiquetas",
      "SELECT DISTINCT etiqueta FROM equipos_red;" },

    // 6.1
    { "equipos ttl=128",
      "SELECT * FROM equipos_red WHERE ttl=128;" },
    // 6.2
    { "equipos notas_detalladas",
      "SELECT ip, notas_detalladas FROM equipos_red "
      "WHERE notas_detalladas IS NOT NULL AND notas_detalladas != '';" },
    // 6.3
    { "equipos sin_hostname",
      "SELECT * FROM equipos_red WHERE hostname IS NULL OR hostname = '';" },
    // 6.4
    { "equipos con_vendor",
      "SELECT * FROM equipos_red WHERE vendor IS NOT NULL AND vendor != '';" },
    // 6.5
    { "equipos con_puertos",
      "SELECT * FROM equipos_red WHERE puertos_abiertos IS NOT NULL AND puertos_abiertos != '';" },

    // 7.1
    { "interfaces",
      "SELECT * FROM interfaces;" },
    // 7.2
    { "interfaces nombre=eth1",
      "SELECT * FROM interfaces WHERE nombre='eth1';" },
    // 7.3
    { "interfaces mac=AA:BB:CC:DD:EE:FF",
      "SELECT * FROM interfaces WHERE mac='AA:BB:CC:DD:EE:FF';" },
    // 7.4
    { "resumen mascaras",
      "SELECT mascara, COUNT(*) FROM interfaces GROUP BY mascara;" },
    // 7.5
    { "resumen interfaz",
      "SELECT interfaz_detectada, COUNT(*) FROM equipos_red GROUP BY interfaz_detectada;" },

    // 8.1
    { "equipos ip_estado",
      "SELECT ip, estado FROM equipos_red;" },
    // 8.2
    { "banners SSH",
      "SELECT ip, banners FROM equipos_red WHERE banners LIKE '%SSH%';" },
    // 8.3
    { "resumen dias",
      "SELECT DATE(fecha) as dia, COUNT(*) FROM equipos_red GROUP BY dia;" },
    // 8.4
    { "equipos sin_vulnerabilidades",
      "SELECT * FROM equipos_red WHERE vulnerabilidades IS NULL OR vulnerabilidades = '';" },
    // 8.5
    { "equipos metodos_activos",
      "SELECT * FROM equipos_red "
      "WHERE metodo_descubrimiento IN ('Nmap', 'ARP', 'Ping');" },

    // 9.1 y 9.2
    { "ip_libre",
      "SELECT ip FROM ips_posibles WHERE ip NOT IN (SELECT ip FROM equipos_red);" }
};

struct DbPattern {
    std::regex  regex;
    std::string sql_template;
};

static const std::vector<DbPattern> db_patterns = {
    // 9.1 rango de IP
    { std::regex(R"(equipos ip=([\d\.]+)-([\d\.]+))"),
      "SELECT * FROM equipos_red WHERE INET_ATON(ip) BETWEEN INET_ATON('%1%') AND INET_ATON('%2%');" },

    // 1.3 genérico por IP
    { std::regex(R"(equipos ip=([\d\.]+))"),
      "SELECT * FROM equipos_red WHERE ip='%1%';" },

    // 1.4 genérico por MAC
    { std::regex(R"(equipos mac=([0-9A-Fa-f:]+))"),
      "SELECT * FROM equipos_red WHERE mac='%1%';" },

    // 1.5 genérico por hostname
    { std::regex(R"(equipos hostname=(\S+))"),
      "SELECT * FROM equipos_red WHERE hostname='%1%';" },

    // 2.1 genérico por etiqueta
    { std::regex(R"(equipos etiqueta=(\S+))"),
      "SELECT * FROM equipos_red WHERE etiqueta='%1%';" },

    // 2.2/2.3 genérico por estado
    { std::regex(R"(equipos estado=(\w+))"),
      "SELECT * FROM equipos_red WHERE estado='%1%';" },

    // 2.4 genérico por SO
    { std::regex(R"(equipos so=(.+))"),
      "SELECT * FROM equipos_red WHERE sistema_operativo LIKE '%%%1%%';" },

    // 2.5 genérico por método
    { std::regex(R"(equipos metodo=(\w+))"),
      "SELECT * FROM equipos_red WHERE metodo_descubrimiento='%1%';" },

    // 3.2 genérico por puerto
    { std::regex(R"(equipos puerto=(\d+))"),
      "SELECT * FROM equipos_red WHERE puertos_abiertos LIKE '%1%';" },

    // 3.3 genérico por interfaz
    { std::regex(R"(equipos interfaz=(\w+))"),
      "SELECT * FROM equipos_red WHERE interfaz_detectada='%1%';" },

    // 3.4 genérico por RTT
    { std::regex(R"(equipos rtt_ms>(\d+))"),
      "SELECT * FROM equipos_red WHERE rtt_ms > %1%;" },

    // 6.1/6.4 genérico por TTL
    { std::regex(R"(equipos ttl=(\d+))"),
      "SELECT * FROM equipos_red WHERE ttl = %1%;" },

    // 4.5 genérico por fecha
    { std::regex(R"(equipos fecha=(\d{4}-\d{2}-\d{2}))"),
      "SELECT * FROM equipos_red WHERE DATE(fecha) = '%1%';" },

    // 5.1 alias
    { std::regex(R"(equipos ultimos7dias)"),
      "SELECT * FROM equipos_red WHERE fecha >= NOW() - INTERVAL 7 DAY;" },

    // SQL libre
    { std::regex(R"(-mysql\s+(.+))"),
      "%1%" }
};

void printTable(sql::ResultSet* res) {
    auto* meta = res->getMetaData();
    int cols = meta->getColumnCount();
    std::vector<int> widths(cols, 0);
    std::vector<std::string> headers;
    // Calcula ancho mínimo de cada columna (nombre de columna)
    for (int i = 1; i <= cols; ++i) {
        std::string col = meta->getColumnName(i);
        widths[i-1] = col.size();
        headers.push_back(col);
    }

    // Calcula el ancho máximo de cada columna
    std::vector<std::vector<std::string>> rows;
    while (res->next()) {
        std::vector<std::string> row;
        for (int i = 1; i <= cols; ++i) {
            std::string v = res->isNull(i) ? "" : res->getString(i);
            if ((int)v.size() > widths[i-1]) widths[i-1] = v.size();
            row.push_back(v);
        }
        rows.push_back(row);
    }

    // Línea separadora
    auto print_sep = [&] {
        std::cout << "+";
        for (int i = 0; i < cols; ++i)
            std::cout << std::string(widths[i]+2, '-') << "+";
        std::cout << "\n";
    };

    // Cabecera
    print_sep();
    std::cout << "|";
    for (int i = 0; i < cols; ++i)
        std::cout << " " << BOLD << headers[i] << std::string(widths[i]-headers[i].size()+1, ' ') << RESET << "|";
    std::cout << "\n";
    print_sep();

    // Filas
    for (const auto& row : rows) {
        std::cout << "|";
        for (int i = 0; i < cols; ++i)
            std::cout << " " << row[i] << std::string(widths[i]-row[i].size()+1, ' ') << "|";
        std::cout << "\n";
    }
    print_sep();
    std::cout << BOLD << "Total: " << rows.size() << " filas.\n" << RESET;
}

static void db(const std::string& sql) {
    try {
        sql::Connection* conn = gBBDD.getConexion();
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(sql));

        // Imprime la tabla 
        printTable(res.get());

    } catch (const sql::SQLException& e) {
        std::cerr << RED << "[DB][ERROR] " << e.what() << RESET << std::endl;
    } catch (const std::exception& e) {
        std::cerr << RED << "[DB][ERROR] " << e.what() << RESET << std::endl;
    }
}


static void print_usage_db() {
    std::cout << BOLD << CYAN << "\nUso del comando db:\n\n" << RESET
              << GREEN << "  db -h, --help             " << RESET << "Muestra esta ayuda\n"
              << GREEN << "  db -mysql <SQL>           " << RESET << "Ejecuta SQL libre\n\n"
              << GREEN << "  Alias disponibles:\n" << RESET;
    for (auto& [alias, _] : db_aliases) {
        std::cout << "    " << BOLD << alias << RESET << "\n";
    }
    std::cout << "\n  Patrones dinámicos:\n" << RESET
              << "    equipos ip=<IP>\n"
              << "    equipos ip=<IP1>-<IP2>\n"
              << "    equipos mac=<MAC>\n"
              << "    equipos hostname=<host>\n"
              << "    equipos etiqueta=<tag>\n"
              << "    equipos estado=<state>\n"
              << "    equipos so=<OS>\n"
              << "    equipos metodo=<method>\n"
              << "    equipos puerto=<num>\n"
              << "    equipos interfaz=<iface>\n"
              << "    equipos rtt_ms>n\n"
              << "    equipos ttl=n\n"
              << "    equipos fecha=YYYY-MM-DD\n"
              << "    equipos ultimos7dias\n"
              << "    db -mysql <SQL>\n\n";
}


void db(const std::string& argumentos) {
    // Trim
    std::string args = argumentos;
    while (!args.empty() && isspace(args.front())) args.erase(args.begin());

    if (args.empty() || args == "-h" || args == "--help") {
        print_usage_db();
        return;
    }

    // Alias fijos
    auto it = db_aliases.find(args);
    if (it != db_aliases.end()) {
        runQuery(it->second);
        return;
    }

    // Patrones
    for (auto& pat : db_patterns) {
        std::smatch m;
        if (std::regex_match(args, m, pat.regex)) {
            std::string sql = pat.sql_template;
            for (size_t i = 1; i < m.size(); ++i) {
                std::string token = "%" + std::to_string(i) + "%";
                sql = std::regex_replace(sql, std::regex(token), m[i].str());
            }
            runQuery(sql);
            return;
        }
    }

    // No reconocido
    std::cout << RED << "[!] Comando db no reconocido: " << RESET << args << "\n"
              << "    Usa 'db -h' para ver ayuda.\n";
}
