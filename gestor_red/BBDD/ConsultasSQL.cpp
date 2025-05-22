#include "ConsultasSQL.hpp"

// Definir consultas
namespace ConsultasSQL {
    const char* INSERT_EQUIPO = "INSERT INTO equipos_red (etiqueta, ip, mac, hostname, metodo_descubrimiento, puertos_abiertos, banners, sistema_operativo, interfaz_detectada, estado, ttl, rtt_ms, notas, vendor, vulnerabilidades, notas_detalladas, fecha) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NOW())";
    const char* INSERT_INTERFAZ = "INSERT INTO interfaces (nombre, ip, mascara, ip_red, broadcast, mac) VALUES (?, ?, ?, ?, ?, ?)";
}
